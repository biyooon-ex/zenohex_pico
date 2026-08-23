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
#include "query.h"
#include "sample.h"
#include "session_option.h"
#include "subscriber.h"
#include "subscriber_option.h"
#include "term.h"

ErlNifResourceType *zxp_session_resource_type = NULL;

typedef struct
{
  pthread_mutex_t mutex;
  pthread_cond_t complete;
  bool is_z_get_complete;
  size_t ref_count;
  ErlNifEnv *env;
  ERL_NIF_TERM replies;
  ERL_NIF_TERM exception;
} zxp_session_get_context_t;

static void zxp_session_get_context_release(zxp_session_get_context_t *context)
{
  bool should_free = false;

  pthread_mutex_lock(&context->mutex);
  context->ref_count--;
  should_free = context->ref_count == 0;
  pthread_mutex_unlock(&context->mutex);

  if (should_free)
  {
    enif_free_env(context->env);
    pthread_cond_destroy(&context->complete);
    pthread_mutex_destroy(&context->mutex);
    enif_free(context);
  }
}

static void zxp_session_get_reply_cb(z_loaned_reply_t *reply, void *arg)
{
  zxp_session_get_context_t *context = arg;

  pthread_mutex_lock(&context->mutex);
  {
    if (enif_is_exception(context->env, context->exception))
    {
      pthread_mutex_unlock(&context->mutex);
      return;
    }

    ERL_NIF_TERM term;
    if (z_reply_is_ok(reply))
    {
      const z_loaned_sample_t *sample = z_reply_ok(reply);
      term = zxp_struct_from_zp_sample(context->env, sample);
    }
    else
    {
      const z_loaned_reply_err_t *reply_err = z_reply_err(reply);
      term = zxp_struct_from_zp_reply_err(context->env, reply_err);
    }

    if (enif_is_exception(context->env, term))
    {
      context->exception = term;
    }
    else
    {
      context->replies = enif_make_list_cell(context->env, term, context->replies);
    }
  }
  pthread_mutex_unlock(&context->mutex);
}

static void zxp_session_get_drop_cb(void *arg)
{
  zxp_session_get_context_t *context = arg;
  pthread_mutex_lock(&context->mutex);
  context->is_z_get_complete = true;
  pthread_cond_signal(&context->complete);
  pthread_mutex_unlock(&context->mutex);
  zxp_session_get_context_release(context);
}

static void zxp_session_get_deadline(uint64_t timeout_ms, struct timespec *deadline)
{
  clock_gettime(CLOCK_MONOTONIC, deadline);
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
    z_drop(z_move(session));
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

ERL_NIF_TERM zxp_session_declare_subscriber(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  z_owned_session_t *session_p = NULL;
  if (argc != 4 || !enif_get_resource(env, argv[0], zxp_session_resource_type, (void **)&session_p))
  {
    return enif_make_badarg(env);
  }

  ErlNifBinary keyexpr_binary;
  ErlNifPid pid;
  if (!enif_inspect_binary(env, argv[1], &keyexpr_binary) ||
      !enif_get_local_pid(env, argv[2], &pid))
  {
    return enif_make_badarg(env);
  }

  z_subscriber_options_t options;
  z_subscriber_options_default(&options);
  if (!zxp_subscriber_options_init(env, argv[3], &options))
  {
    return enif_make_badarg(env);
  }

  z_owned_keyexpr_t keyexpr;
  z_result_t ret =
      z_keyexpr_from_substr(&keyexpr, (const char *)keyexpr_binary.data, keyexpr_binary.size);
  if (ret != Z_OK)
  {
    return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
  }

  zxp_subscriber_t *subscriber =
      enif_alloc_resource(zxp_subscriber_resource_type, sizeof(*subscriber));
  if (subscriber == NULL)
  {
    z_drop(z_move(keyexpr));
    return zxp_raise_null_pointer(env, __FILE__, __LINE__);
  }
  *subscriber = (zxp_subscriber_t){0};
  z_internal_null(&subscriber->subscriber);

  if (!zxp_subscriber_init(subscriber, &pid))
  {
    enif_release_resource(subscriber);
    z_drop(z_move(keyexpr));
    return zxp_raise_null_pointer(env, __FILE__, __LINE__);
  }

  z_owned_closure_sample_t callback;
  ret = z_closure_sample(
      &callback, zxp_subscriber_sample_cb, zxp_subscriber_drop_cb, subscriber->context);
  if (ret != Z_OK)
  {
    zxp_subscriber_shutdown(subscriber, false);
    enif_release_resource(subscriber);
    z_drop(z_move(keyexpr));
    return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
  }

  z_owned_subscriber_t owned_subscriber;
  z_internal_null(&owned_subscriber);
  ret = z_declare_subscriber(
      z_loan(*session_p), &owned_subscriber, z_loan(keyexpr), z_move(callback), &options);
  z_drop(z_move(keyexpr));
  if (ret != Z_OK)
  {
    zxp_subscriber_shutdown(subscriber, false);
    enif_release_resource(subscriber);
    return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
  }

  z_take(&subscriber->subscriber, z_move(owned_subscriber));
  ERL_NIF_TERM subscriber_ref = enif_make_resource(env, subscriber);
  enif_release_resource(subscriber);
  return enif_make_tuple2(env, ok_atom, subscriber_ref);
}

ERL_NIF_TERM zxp_session_put(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  z_owned_session_t *session_p = NULL;
  if (argc != 4 || !enif_get_resource(env, argv[0], zxp_session_resource_type, (void **)&session_p))
  {
    return enif_make_badarg(env);
  }

  ErlNifBinary keyexpr_binary;
  if (!enif_inspect_binary(env, argv[1], &keyexpr_binary))
  {
    return enif_make_badarg(env);
  }

  ErlNifBinary payload_binary;
  if (!enif_inspect_binary(env, argv[2], &payload_binary))
  {
    return enif_make_badarg(env);
  }

  zxp_session_put_options_t *put_options = zxp_session_put_options_new(env, argv[3]);
  if (put_options == NULL)
  {
    return enif_make_badarg(env);
  }

  z_owned_keyexpr_t keyexpr;
  z_result_t ret =
      z_keyexpr_from_substr(&keyexpr, (const char *)keyexpr_binary.data, keyexpr_binary.size);
  if (ret != Z_OK)
  {
    zxp_session_put_options_drop(put_options);
    return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
  }

  z_owned_bytes_t payload;
  z_bytes_from_buf(&payload, payload_binary.data, payload_binary.size, NULL, NULL);

  ret = z_put(z_loan(*session_p),
              z_loan(keyexpr),
              z_move(payload),
              zxp_session_put_options_loan(put_options));
  z_drop(z_move(keyexpr));
  zxp_session_put_options_drop(put_options);
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

  zxp_session_get_options_t *get_options = zxp_session_get_options_new(env, argv[3]);
  if (get_options == NULL)
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
    zxp_session_get_options_drop(get_options);
    return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
  }

  const char *parameters = query_separator == NULL ? NULL : (const char *)(query_separator + 1);

  zxp_session_get_context_t *context = enif_alloc(sizeof(*context));
  if (context == NULL)
  {
    z_drop(z_move(keyexpr));
    zxp_session_get_options_drop(get_options);
    return zxp_raise_null_pointer(env, __FILE__, __LINE__);
  }

  memset(context, 0, sizeof(*context));
  if (pthread_mutex_init(&context->mutex, NULL) != 0)
  {
    enif_free(context);
    z_drop(z_move(keyexpr));
    zxp_session_get_options_drop(get_options);
    return zxp_raise_null_pointer(env, __FILE__, __LINE__);
  }

  pthread_condattr_t attr;
  if (pthread_condattr_init(&attr) != 0)
  {
    pthread_mutex_destroy(&context->mutex);
    enif_free(context);
    z_drop(z_move(keyexpr));
    zxp_session_get_options_drop(get_options);
    return zxp_raise_null_pointer(env, __FILE__, __LINE__);
  }

  int condattr_result = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
  int cond_result =
      condattr_result == 0 ? pthread_cond_init(&context->complete, &attr) : condattr_result;
  pthread_condattr_destroy(&attr);
  if (cond_result != 0)
  {
    pthread_mutex_destroy(&context->mutex);
    enif_free(context);
    z_drop(z_move(keyexpr));
    zxp_session_get_options_drop(get_options);
    return zxp_raise_null_pointer(env, __FILE__, __LINE__);
  }

  context->env = enif_alloc_env();
  if (context->env == NULL)
  {
    pthread_cond_destroy(&context->complete);
    pthread_mutex_destroy(&context->mutex);
    enif_free(context);
    z_drop(z_move(keyexpr));
    zxp_session_get_options_drop(get_options);
    return zxp_raise_null_pointer(env, __FILE__, __LINE__);
  }

  context->replies = enif_make_list(context->env, 0);
  context->exception = nil_atom;
  context->ref_count = 1;

  z_owned_closure_reply_t callback;
  ret = z_closure_reply(&callback, zxp_session_get_reply_cb, zxp_session_get_drop_cb, context);
  if (ret != Z_OK)
  {
    zxp_session_get_context_release(context);
    z_drop(z_move(keyexpr));
    zxp_session_get_options_drop(get_options);
    return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
  }

  pthread_mutex_lock(&context->mutex);
  context->ref_count++;
  pthread_mutex_unlock(&context->mutex);

  ret = z_get_with_parameters_substr(z_loan(*session_p),
                                     z_loan(keyexpr),
                                     parameters,
                                     parameters_length,
                                     z_move(callback),
                                     zxp_session_get_options_loan(get_options));
  z_drop(z_move(keyexpr));
  zxp_session_get_options_drop(get_options);
  if (ret != Z_OK)
  {
    zxp_session_get_context_release(context);
    return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
  }

  struct timespec deadline;
  zxp_session_get_deadline(timeout_ms, &deadline);

  pthread_mutex_lock(&context->mutex);
  while (!context->is_z_get_complete)
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
      return zxp_raise(env, __FILE__, __LINE__, "pthread_cond_timedwait/3 failed");
    }
  }

  if (enif_is_exception(context->env, context->exception))
  {
    ERL_NIF_TERM term = enif_make_copy(env, context->exception);
    pthread_mutex_unlock(&context->mutex);
    zxp_session_get_context_release(context);
    return term;
  }

  ERL_NIF_TERM replies = enif_make_copy(env, context->replies);
  pthread_mutex_unlock(&context->mutex);
  zxp_session_get_context_release(context);

  unsigned reply_count = 0;
  if (!enif_get_list_length(env, replies, &reply_count))
  {
    return zxp_raise(env, __FILE__, __LINE__, "enif_get_list_length/3 failed");
  }

  if (reply_count == 0)
  {
    return enif_make_tuple2(env, error_atom, timeout_atom);
  }

  ERL_NIF_TERM ordered_replies;
  if (!enif_make_reverse_list(env, replies, &ordered_replies))
  {
    return zxp_raise(env, __FILE__, __LINE__, "enif_make_reverse_list/3 failed");
  }

  return enif_make_tuple2(env, ok_atom, ordered_replies);
}
