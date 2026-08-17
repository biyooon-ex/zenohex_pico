#define _POSIX_C_SOURCE 200809L

#include <erl_nif.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zenoh-pico.h>

#include "config.h"
#include "macro.h"
#include "term.h"

ErlNifResourceType *zxp_session_resource_type = NULL;

typedef struct
{
  bool is_error;
  uint8_t *attachment;
  size_t attachment_len;
  uint8_t *payload;
  size_t payload_len;
  char *encoding;
  char *key_expr;
  z_congestion_control_t congestion_control;
  bool express;
  z_sample_kind_t kind;
  z_priority_t priority;
} zxp_session_get_reply_t;

typedef struct
{
  pthread_mutex_t mutex;
  pthread_cond_t complete;
  size_t ref_count;
  bool is_complete;
  bool allocation_failed;
  zxp_session_get_reply_t *replies;
  size_t reply_count;
  size_t reply_capacity;
} zxp_session_get_context_t;

static void zxp_session_get_reply_clear(zxp_session_get_reply_t *reply)
{
  free(reply->attachment);
  free(reply->payload);
  free(reply->encoding);
  free(reply->key_expr);
  memset(reply, 0, sizeof(*reply));
}

static void zxp_session_get_context_release(zxp_session_get_context_t *context)
{
  bool should_free = false;

  pthread_mutex_lock(&context->mutex);
  context->ref_count--;
  should_free = context->ref_count == 0;
  pthread_mutex_unlock(&context->mutex);

  if (should_free)
  {
    for (size_t index = 0; index < context->reply_count; index++)
    {
      zxp_session_get_reply_clear(&context->replies[index]);
    }
    free(context->replies);
    pthread_cond_destroy(&context->complete);
    pthread_mutex_destroy(&context->mutex);
    free(context);
  }
}

static bool zxp_session_get_copy_buffer(const uint8_t *source, size_t length, uint8_t **destination)
{
  *destination = NULL;
  if (length == 0)
  {
    return true;
  }

  *destination = malloc(length);
  if (*destination == NULL)
  {
    return false;
  }
  memcpy(*destination, source, length);
  return true;
}

static bool zxp_session_get_copy_bytes(const z_loaned_bytes_t *source, uint8_t **destination,
                                       size_t *length)
{
  z_owned_slice_t slice;
  if (z_bytes_to_slice(source, &slice) != Z_OK)
  {
    return false;
  }

  *length = z_slice_len(z_loan(slice));
  bool copied = zxp_session_get_copy_buffer(z_slice_data(z_loan(slice)), *length, destination);
  z_drop(z_move(slice));
  return copied;
}

static bool zxp_session_get_copy_string(const z_loaned_string_t *source, char **destination)
{
  size_t length = z_string_len(source);
  *destination = malloc(length + 1);
  if (*destination == NULL)
  {
    return false;
  }
  memcpy(*destination, z_string_data(source), length);
  (*destination)[length] = '\0';
  return true;
}

static bool zxp_session_get_copy_encoding(const z_loaned_encoding_t *encoding, char **destination)
{
  z_owned_string_t string;
  if (z_encoding_to_string(encoding, &string) != Z_OK)
  {
    return false;
  }

  bool copied = zxp_session_get_copy_string(z_loan(string), destination);
  z_drop(z_move(string));
  return copied;
}

static bool zxp_session_get_copy_keyexpr(const z_loaned_keyexpr_t *keyexpr, char **destination)
{
  z_view_string_t string;
  if (z_keyexpr_as_view_string(keyexpr, &string) != Z_OK)
  {
    return false;
  }
  return zxp_session_get_copy_string(z_loan(string), destination);
}

static bool zxp_session_get_copy_reply(z_loaned_reply_t *reply, zxp_session_get_reply_t *result)
{
  memset(result, 0, sizeof(*result));

  if (!z_reply_is_ok(reply))
  {
    const z_loaned_reply_err_t *error = z_reply_err(reply);
    result->is_error = true;
    return zxp_session_get_copy_bytes(
               z_reply_err_payload(error), &result->payload, &result->payload_len) &&
           zxp_session_get_copy_encoding(z_reply_err_encoding(error), &result->encoding);
  }

  const z_loaned_sample_t *sample = z_reply_ok(reply);
  result->congestion_control = z_sample_congestion_control(sample);
  result->express = z_sample_express(sample);
  result->kind = z_sample_kind(sample);
  result->priority = z_sample_priority(sample);

  const z_loaned_bytes_t *attachment = z_sample_attachment(sample);
  return zxp_session_get_copy_keyexpr(z_sample_keyexpr(sample), &result->key_expr) &&
         zxp_session_get_copy_bytes(
             z_sample_payload(sample), &result->payload, &result->payload_len) &&
         zxp_session_get_copy_encoding(z_sample_encoding(sample), &result->encoding) &&
         (attachment == NULL ||
          zxp_session_get_copy_bytes(attachment, &result->attachment, &result->attachment_len));
}

static void zxp_session_get_reply_handler(z_loaned_reply_t *reply, void *arg)
{
  zxp_session_get_reply_t copied_reply;
  if (!zxp_session_get_copy_reply(reply, &copied_reply))
  {
    zxp_session_get_reply_clear(&copied_reply);
    pthread_mutex_lock(&((zxp_session_get_context_t *)arg)->mutex);
    ((zxp_session_get_context_t *)arg)->allocation_failed = true;
    pthread_mutex_unlock(&((zxp_session_get_context_t *)arg)->mutex);
    return;
  }

  zxp_session_get_context_t *context = arg;
  pthread_mutex_lock(&context->mutex);
  if (context->reply_count == context->reply_capacity)
  {
    size_t capacity = context->reply_capacity == 0 ? 4 : context->reply_capacity * 2;
    zxp_session_get_reply_t *replies = realloc(context->replies, capacity * sizeof(*replies));
    if (replies == NULL)
    {
      context->allocation_failed = true;
      pthread_mutex_unlock(&context->mutex);
      zxp_session_get_reply_clear(&copied_reply);
      return;
    }
    context->replies = replies;
    context->reply_capacity = capacity;
  }
  context->replies[context->reply_count++] = copied_reply;
  pthread_mutex_unlock(&context->mutex);
}

static void zxp_session_get_reply_dropper(void *arg)
{
  zxp_session_get_context_t *context = arg;
  pthread_mutex_lock(&context->mutex);
  context->is_complete = true;
  pthread_cond_signal(&context->complete);
  pthread_mutex_unlock(&context->mutex);
  zxp_session_get_context_release(context);
}

static ERL_NIF_TERM zxp_session_get_binary(ErlNifEnv *env, const uint8_t *data, size_t length)
{
  ERL_NIF_TERM binary;
  uint8_t *destination = enif_make_new_binary(env, length, &binary);
  if (length > 0)
  {
    memcpy(destination, data, length);
  }
  return binary;
}

static ERL_NIF_TERM zxp_session_get_congestion_control(ErlNifEnv *env, z_congestion_control_t value)
{
  return enif_make_atom(env, value == Z_CONGESTION_CONTROL_BLOCK ? "block" : "drop");
}

static ERL_NIF_TERM zxp_session_get_priority(ErlNifEnv *env, z_priority_t value)
{
  static const char *names[] = {"control",
                                "real_time",
                                "interactive_high",
                                "interactive_low",
                                "data_high",
                                "data",
                                "data_low",
                                "background"};
  return enif_make_atom(env, names[value]);
}

static ERL_NIF_TERM zxp_session_get_reply_term(ErlNifEnv *env, const zxp_session_get_reply_t *reply)
{
  if (reply->is_error)
  {
    ERL_NIF_TERM keys[] = {enif_make_atom(env, "__struct__"),
                           enif_make_atom(env, "payload"),
                           enif_make_atom(env, "encoding")};
    ERL_NIF_TERM values[] = {enif_make_atom(env, "Elixir.ZenohexPico.Query.ReplyError"),
                             zxp_session_get_binary(env, reply->payload, reply->payload_len),
                             enif_make_string(env, reply->encoding, ERL_NIF_LATIN1)};
    ERL_NIF_TERM term;
    enif_make_map_from_arrays(env, keys, values, 3, &term);
    return term;
  }

  ERL_NIF_TERM keys[] = {enif_make_atom(env, "__struct__"),
                         enif_make_atom(env, "attachment"),
                         enif_make_atom(env, "congestion_control"),
                         enif_make_atom(env, "encoding"),
                         enif_make_atom(env, "express"),
                         enif_make_atom(env, "key_expr"),
                         enif_make_atom(env, "kind"),
                         enif_make_atom(env, "payload"),
                         enif_make_atom(env, "priority"),
                         enif_make_atom(env, "timestamp")};
  ERL_NIF_TERM values[] = {
      enif_make_atom(env, "Elixir.ZenohexPico.Sample"),
      reply->attachment == NULL
          ? enif_make_atom(env, "nil")
          : zxp_session_get_binary(env, reply->attachment, reply->attachment_len),
      zxp_session_get_congestion_control(env, reply->congestion_control),
      enif_make_string(env, reply->encoding, ERL_NIF_LATIN1),
      enif_make_atom(env, reply->express ? "true" : "false"),
      enif_make_string(env, reply->key_expr, ERL_NIF_LATIN1),
      enif_make_atom(env, reply->kind == Z_SAMPLE_KIND_DELETE ? "delete" : "put"),
      zxp_session_get_binary(env, reply->payload, reply->payload_len),
      zxp_session_get_priority(env, reply->priority),
      enif_make_atom(env, "nil")};
  ERL_NIF_TERM term;
  enif_make_map_from_arrays(env, keys, values, 10, &term);
  return term;
}

static bool zxp_session_get_options(ErlNifEnv *env, ERL_NIF_TERM term, z_get_options_t *options)
{
  ERL_NIF_TERM head;
  ERL_NIF_TERM tail;
  ERL_NIF_TERM query_timeout_atom = enif_make_atom(env, "query_timeout");

  while (enif_get_list_cell(env, term, &head, &tail))
  {
    const ERL_NIF_TERM *tuple;
    int arity;
    if (!enif_get_tuple(env, head, &arity, &tuple) || arity != 2)
    {
      return false;
    }

    if (!enif_is_identical(tuple[0], query_timeout_atom) ||
        !enif_get_uint64(env, tuple[1], &options->timeout_ms))
    {
      return false;
    }
    term = tail;
  }

  return enif_is_empty_list(env, term);
}

static void zxp_session_get_deadline(uint64_t timeout_ms, struct timespec *deadline)
{
  clock_gettime(CLOCK_REALTIME, deadline);
  deadline->tv_sec += (time_t)(timeout_ms / 1000);
  deadline->tv_nsec += (long)((timeout_ms % 1000) * 1000000);
  if (deadline->tv_nsec >= 1000000000L)
  {
    deadline->tv_sec++;
    deadline->tv_nsec -= 1000000000L;
  }
}

static void zxp_session_dtor(ErlNifEnv *env, void *obj)
{
  UNUSED(env);

  z_owned_session_t *session_p = (z_owned_session_t *)obj;
  z_drop(z_move(*session_p));
}

static const ErlNifResourceTypeInit ZxpSessionResourceTypeInit = {
    .dtor = zxp_session_dtor,
    .members = 1,
};

void zxp_session_enif_init_resource_type(ErlNifEnv *env)
{
  zxp_session_resource_type = enif_init_resource_type(
      env, "zxp_session", &ZxpSessionResourceTypeInit, ERL_NIF_RT_CREATE, NULL);
}

ERL_NIF_TERM zxp_session_open(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  UNUSED(argc);

  z_owned_config_t *config_p = NULL;
  if (!enif_get_resource(env, argv[0], zxp_config_resource_type, (void **)&config_p))
  {
    return enif_make_badarg(env);
  }

  z_owned_config_t config;
  {
    z_result_t ret = z_clone(&config, z_loan(*config_p));

    if (ret != Z_OK)
    {
      return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
    }
  }

  z_owned_session_t session;
  {
    z_result_t ret = z_open(&session, z_move(config), NULL);

    if (ret != Z_OK)
    {
      return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
    }
  }

  z_owned_session_t *session_p =
      enif_alloc_resource(zxp_session_resource_type, sizeof(z_owned_session_t));
  if (session_p == NULL)
  {
    return zxp_raise_null_pointer(env, __FILE__, __LINE__);
  }

  z_internal_null(session_p);
  z_take(session_p, z_move(session));

  ERL_NIF_TERM session_ref = enif_make_resource(env, session_p);
  enif_release_resource(session_p);

  return enif_make_tuple2(env, ok_atom, session_ref);
}

ERL_NIF_TERM zxp_session_close(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  UNUSED(argc);

  z_owned_session_t *session_p = NULL;
  if (!enif_get_resource(env, argv[0], zxp_session_resource_type, (void **)&session_p))
  {
    return enif_make_badarg(env);
  }

  z_result_t ret = z_close(z_loan_mut(*session_p), NULL);
  if (ret != Z_OK)
  {
    return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
  }

  return ok_atom;
}

ERL_NIF_TERM zxp_session_get(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  UNUSED(argc);

  z_owned_session_t *session_p = NULL;
  if (!enif_get_resource(env, argv[0], zxp_session_resource_type, (void **)&session_p))
  {
    return enif_make_badarg(env);
  }

  ErlNifBinary selector;
  if (!enif_inspect_binary(env, argv[1], &selector))
  {
    return enif_make_badarg(env);
  }

  ErlNifUInt64 timeout_ms;
  if (!enif_get_uint64(env, argv[2], &timeout_ms))
  {
    return enif_make_badarg(env);
  }

  z_get_options_t options;
  z_get_options_default(&options);
  if (!zxp_session_get_options(env, argv[3], &options))
  {
    return enif_make_badarg(env);
  }

  const uint8_t *query_separator = memchr(selector.data, '?', selector.size);
  size_t keyexpr_length =
      query_separator == NULL ? selector.size : (size_t)(query_separator - selector.data);
  size_t parameters_length = query_separator == NULL ? 0 : selector.size - keyexpr_length - 1;

  z_owned_keyexpr_t keyexpr;
  z_result_t ret = z_keyexpr_from_substr(&keyexpr, (const char *)selector.data, keyexpr_length);
  if (ret != Z_OK)
  {
    return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
  }

  char *parameters = malloc(parameters_length + 1);
  if (parameters == NULL)
  {
    z_drop(z_move(keyexpr));
    return zxp_raise_null_pointer(env, __FILE__, __LINE__);
  }
  if (parameters_length > 0)
  {
    memcpy(parameters, query_separator + 1, parameters_length);
  }
  parameters[parameters_length] = '\0';

  zxp_session_get_context_t *context = calloc(1, sizeof(*context));
  if (context == NULL || pthread_mutex_init(&context->mutex, NULL) != 0 ||
      pthread_cond_init(&context->complete, NULL) != 0)
  {
    free(context);
    free(parameters);
    z_drop(z_move(keyexpr));
    return zxp_raise_null_pointer(env, __FILE__, __LINE__);
  }
  context->ref_count = 1;

  z_owned_closure_reply_t callback;
  pthread_mutex_lock(&context->mutex);
  context->ref_count++;
  pthread_mutex_unlock(&context->mutex);
  ret = z_closure_reply(
      &callback, zxp_session_get_reply_handler, zxp_session_get_reply_dropper, context);
  if (ret != Z_OK)
  {
    zxp_session_get_context_release(context);
    zxp_session_get_context_release(context);
    free(parameters);
    z_drop(z_move(keyexpr));
    return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
  }

  ret = z_get(z_loan(*session_p), z_loan(keyexpr), parameters, z_move(callback), &options);
  free(parameters);
  z_drop(z_move(keyexpr));
  if (ret != Z_OK)
  {
    zxp_session_get_context_release(context);
    return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
  }

  struct timespec deadline;
  zxp_session_get_deadline(timeout_ms, &deadline);

  pthread_mutex_lock(&context->mutex);
  while (!context->is_complete)
  {
    int wait_result = pthread_cond_timedwait(&context->complete, &context->mutex, &deadline);
    if (wait_result == ETIMEDOUT)
    {
      break;
    }
    if (wait_result != 0)
    {
      pthread_mutex_unlock(&context->mutex);
      zxp_session_get_context_release(context);
      return zxp_raise(env, __FILE__, __LINE__, "pthread_cond_timedwait failed");
    }
  }

  if (context->allocation_failed)
  {
    pthread_mutex_unlock(&context->mutex);
    zxp_session_get_context_release(context);
    return zxp_raise_null_pointer(env, __FILE__, __LINE__);
  }

  ERL_NIF_TERM replies = enif_make_list(env, 0);
  for (size_t index = context->reply_count; index > 0; index--)
  {
    replies = enif_make_list_cell(
        env, zxp_session_get_reply_term(env, &context->replies[index - 1]), replies);
  }
  bool has_replies = context->reply_count > 0;
  pthread_mutex_unlock(&context->mutex);
  zxp_session_get_context_release(context);

  if (!has_replies)
  {
    return enif_make_tuple2(env, error_atom, enif_make_atom(env, "timeout"));
  }
  return enif_make_tuple2(env, ok_atom, replies);
}
