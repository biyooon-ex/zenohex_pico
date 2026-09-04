#include <defaultatoms.h>
#include <errno.h>
#include <erl_nif_priv.h>
#include <memory.h>
#include <nifs.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <term.h>
#include <time.h>
#include <zenoh-pico.h>

#include "config.h"
#include "query.h"
#include "sample.h"
#include "session.h"
#include "session_option.h"

ErlNifResourceType *zxp_session_resource_type = NULL;
static term session_closed_atom;
static term timeout_atom;

typedef struct {
  pthread_mutex_t mutex;
  z_owned_session_t session;
  bool is_mutex_initialized;
} zxp_session_resource_t;

typedef struct {
  bool is_ok;
  union {
    zxp_sample_t sample;
    zxp_reply_error_t reply_error;
  } value;
} zxp_session_reply_t;

typedef struct {
  pthread_mutex_t mutex;
  pthread_cond_t complete;
  bool is_z_get_complete;
  bool has_allocation_error;
  size_t ref_count;
  size_t reply_count;
  size_t reply_capacity;
  zxp_session_reply_t *replies;
} zxp_session_get_context_t;

static term zxp_session_error_tuple(Context *ctx, const char *reason)
{
  size_t reason_size = strlen(reason);
  if (memory_ensure_free(ctx, term_binary_heap_size(reason_size) + TUPLE_SIZE(2)) != MEMORY_GC_OK) {
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  term reason_term = term_from_literal_binary(reason, reason_size, &ctx->heap, ctx->global);
  term result = term_alloc_tuple(2, &ctx->heap);
  term_put_tuple_element(result, 0, ERROR_ATOM);
  term_put_tuple_element(result, 1, reason_term);
  return result;
}

static void zxp_session_get_context_release(zxp_session_get_context_t *context)
{
  bool should_free = false;
  pthread_mutex_lock(&context->mutex);
  context->ref_count--;
  should_free = context->ref_count == 0;
  pthread_mutex_unlock(&context->mutex);

  if (should_free) {
    for (size_t index = 0; index < context->reply_count; index++) {
      if (context->replies[index].is_ok) {
        zxp_sample_drop(&context->replies[index].value.sample);
      } else {
        zxp_reply_error_drop(&context->replies[index].value.reply_error);
      }
    }
    free(context->replies);
    pthread_cond_destroy(&context->complete);
    pthread_mutex_destroy(&context->mutex);
    free(context);
  }
}

static bool zxp_session_copy_bytes(zxp_bytes_t *destination, const z_loaned_bytes_t *source)
{
  destination->data = NULL;
  destination->size = 0;
  if (source == NULL) return true;

  z_owned_slice_t slice;
  z_internal_null(&slice);
  if (z_bytes_to_slice(source, &slice) != Z_OK) return false;
  destination->size = z_slice_len(z_loan(slice));
  destination->data = malloc(destination->size == 0 ? 1 : destination->size);
  if (destination->data != NULL && destination->size != 0) {
    memcpy(destination->data, z_slice_data(z_loan(slice)), destination->size);
  }
  z_drop(z_move(slice));
  return destination->data != NULL;
}

static bool zxp_session_copy_encoding(zxp_bytes_t *destination, const z_loaned_encoding_t *source)
{
  destination->data = NULL;
  destination->size = 0;
  if (source == NULL) return true;

  z_owned_string_t string;
  z_internal_null(&string);
  if (z_encoding_to_string(source, &string) != Z_OK) return false;
  destination->size = z_string_len(z_loan(string));
  destination->data = malloc(destination->size == 0 ? 1 : destination->size);
  if (destination->data != NULL && destination->size != 0) {
    memcpy(destination->data, z_string_data(z_loan(string)), destination->size);
  }
  z_drop(z_move(string));
  return destination->data != NULL;
}

static bool zxp_session_copy_keyexpr(zxp_bytes_t *destination, const z_loaned_keyexpr_t *source)
{
  destination->data = NULL;
  destination->size = 0;
  z_view_string_t string;
  if (z_keyexpr_as_view_string(source, &string) != Z_OK) return false;
  destination->size = z_string_len(z_loan(string));
  destination->data = malloc(destination->size == 0 ? 1 : destination->size);
  if (destination->data != NULL && destination->size != 0) {
    memcpy(destination->data, z_string_data(z_loan(string)), destination->size);
  }
  return destination->data != NULL;
}

static bool zxp_session_copy_sample(zxp_sample_t *destination, const z_loaned_sample_t *source)
{
  memset(destination, 0, sizeof(*destination));
  destination->congestion_control = z_sample_congestion_control(source);
  destination->express = z_sample_express(source);
  destination->kind = z_sample_kind(source);
  destination->priority = z_sample_priority(source);
  const z_timestamp_t *timestamp = z_sample_timestamp(source);
  if (timestamp != NULL) {
    destination->has_timestamp = true;
    destination->timestamp = *timestamp;
  }
  if (!zxp_session_copy_bytes(&destination->attachment, z_sample_attachment(source)) ||
      !zxp_session_copy_encoding(&destination->encoding, z_sample_encoding(source)) ||
      !zxp_session_copy_keyexpr(&destination->key_expr, z_sample_keyexpr(source)) ||
      !zxp_session_copy_bytes(&destination->payload, z_sample_payload(source))) {
    zxp_sample_drop(destination);
    return false;
  }
  return true;
}

static bool zxp_session_copy_reply_error(zxp_reply_error_t *destination,
    const z_loaned_reply_err_t *source)
{
  memset(destination, 0, sizeof(*destination));
  if (!zxp_session_copy_bytes(&destination->payload, z_reply_err_payload(source)) ||
      !zxp_session_copy_encoding(&destination->encoding, z_reply_err_encoding(source))) {
    zxp_reply_error_drop(destination);
    return false;
  }
  return true;
}

static void zxp_session_get_reply_cb(z_loaned_reply_t *reply, void *arg)
{
  zxp_session_get_context_t *context = arg;
  pthread_mutex_lock(&context->mutex);
  if (context->has_allocation_error) {
    pthread_mutex_unlock(&context->mutex);
    return;
  }

  if (context->reply_count == context->reply_capacity) {
    size_t capacity = context->reply_capacity == 0 ? 4 : context->reply_capacity * 2;
    zxp_session_reply_t *replies = realloc(context->replies, capacity * sizeof(*replies));
    if (replies == NULL) {
      context->has_allocation_error = true;
      pthread_mutex_unlock(&context->mutex);
      return;
    }
    context->replies = replies;
    context->reply_capacity = capacity;
  }

  zxp_session_reply_t *destination = &context->replies[context->reply_count];
  destination->is_ok = z_reply_is_ok(reply);
  bool copied = destination->is_ok ? zxp_session_copy_sample(&destination->value.sample, z_reply_ok(reply)) :
      zxp_session_copy_reply_error(&destination->value.reply_error, z_reply_err(reply));
  if (!copied) {
    context->has_allocation_error = true;
  } else {
    context->reply_count++;
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

static bool zxp_session_get_deadline(uint64_t timeout_ms, struct timespec *deadline)
{
  if (clock_gettime(CLOCK_MONOTONIC, deadline) != 0) return false;
  deadline->tv_sec += (time_t) (timeout_ms / 1000);
  deadline->tv_nsec += (long) ((timeout_ms % 1000) * 1000000);
  if (deadline->tv_nsec >= 1000000000L) {
    deadline->tv_sec++;
    deadline->tv_nsec -= 1000000000L;
  }
  return true;
}

static int zxp_session_get_timedwait(zxp_session_get_context_t *context,
    const struct timespec *monotonic_deadline)
{
  struct timespec monotonic_now, realtime_now, realtime_deadline;
  if (clock_gettime(CLOCK_MONOTONIC, &monotonic_now) != 0 ||
      clock_gettime(CLOCK_REALTIME, &realtime_now) != 0) return EINVAL;
  int64_t remaining_ns = ((int64_t) monotonic_deadline->tv_sec - monotonic_now.tv_sec) * 1000000000LL +
      ((int64_t) monotonic_deadline->tv_nsec - monotonic_now.tv_nsec);
  if (remaining_ns <= 0) return ETIMEDOUT;
  realtime_deadline.tv_sec = realtime_now.tv_sec + remaining_ns / 1000000000LL;
  realtime_deadline.tv_nsec = realtime_now.tv_nsec + remaining_ns % 1000000000LL;
  if (realtime_deadline.tv_nsec >= 1000000000L) {
    realtime_deadline.tv_sec++;
    realtime_deadline.tv_nsec -= 1000000000L;
  }
  return pthread_cond_timedwait(&context->complete, &context->mutex, &realtime_deadline);
}

static void zxp_session_dtor(ErlNifEnv *env, void *obj)
{
  (void) env;

  zxp_session_resource_t *resource = obj;
  if (!resource->is_mutex_initialized) {
    return;
  }

  z_owned_session_t session;
  z_internal_null(&session);
  pthread_mutex_lock(&resource->mutex);
  if (z_internal_session_check(&resource->session)) {
    z_take(&session, z_move(resource->session));
  }
  pthread_mutex_unlock(&resource->mutex);

  if (z_internal_session_check(&session)) {
    z_drop(z_move(session));
  }
  pthread_mutex_destroy(&resource->mutex);
}

static const ErlNifResourceTypeInit ZxpSessionResourceTypeInit = {
  .dtor = zxp_session_dtor,
  .members = 1,
};

bool zxp_session_enif_init_resource_type(ErlNifEnv *env)
{
  zxp_session_resource_type = enif_init_resource_type(
      env, "zxp_session", &ZxpSessionResourceTypeInit, ERL_NIF_RT_CREATE, NULL);
  session_closed_atom = globalcontext_make_atom(env->global, ATOM_STR("\xE", "session_closed"));
  timeout_atom = globalcontext_make_atom(env->global, ATOM_STR("\x7", "timeout"));
  return zxp_session_resource_type != NULL;
}

term zxp_session_open(Context *ctx, int argc, term argv[])
{
  (void) argc;

  ErlNifEnv *env = erl_nif_env_from_context(ctx);
  z_owned_config_t *config_resource = NULL;
  if (!enif_get_resource(env, argv[0], zxp_config_resource_type, (void **) &config_resource)) {
    RAISE_ERROR(BADARG_ATOM);
  }

  z_owned_config_t config;
  z_internal_null(&config);
  if (z_clone(&config, z_loan(*config_resource)) != Z_OK) {
    return zxp_session_error_tuple(ctx, "z_clone");
  }

  z_owned_session_t session;
  z_internal_null(&session);
  if (z_open(&session, z_move(config), NULL) != Z_OK) {
    return zxp_session_error_tuple(ctx, "z_open");
  }

  zxp_session_resource_t *resource =
      enif_alloc_resource(zxp_session_resource_type, sizeof(*resource));
  if (resource == NULL) {
    z_drop(z_move(session));
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }
  z_internal_null(&resource->session);
  resource->is_mutex_initialized = false;
  if (pthread_mutex_init(&resource->mutex, NULL) != 0) {
    z_drop(z_move(session));
    enif_release_resource(resource);
    return zxp_session_error_tuple(ctx, "pthread_mutex_init");
  }

  resource->is_mutex_initialized = true;
  z_take(&resource->session, z_move(session));
  if (memory_ensure_free_with_roots(ctx, TERM_BOXED_RESOURCE_SIZE + TUPLE_SIZE(2), argc, argv,
          MEMORY_CAN_SHRINK) != MEMORY_GC_OK) {
    enif_release_resource(resource);
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  term session_ref = term_from_resource(resource, &ctx->heap);
  enif_release_resource(resource);
  term result = term_alloc_tuple(2, &ctx->heap);
  term_put_tuple_element(result, 0, OK_ATOM);
  term_put_tuple_element(result, 1, session_ref);
  return result;
}

term zxp_session_close(Context *ctx, int argc, term argv[])
{
  (void) argc;

  ErlNifEnv *env = erl_nif_env_from_context(ctx);
  zxp_session_resource_t *resource = NULL;
  if (!enif_get_resource(env, argv[0], zxp_session_resource_type, (void **) &resource)) {
    RAISE_ERROR(BADARG_ATOM);
  }

  z_owned_session_t session;
  z_internal_null(&session);
  pthread_mutex_lock(&resource->mutex);
  if (!z_internal_session_check(&resource->session)) {
    pthread_mutex_unlock(&resource->mutex);
    if (memory_ensure_free_with_roots(ctx, TUPLE_SIZE(2), argc, argv, MEMORY_CAN_SHRINK) !=
        MEMORY_GC_OK) {
      RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    }
    term result = term_alloc_tuple(2, &ctx->heap);
    term_put_tuple_element(result, 0, ERROR_ATOM);
    term_put_tuple_element(result, 1, session_closed_atom);
    return result;
  }
  z_take(&session, z_move(resource->session));
  pthread_mutex_unlock(&resource->mutex);

  z_drop(z_move(session));
  return OK_ATOM;
}

term zxp_session_put(Context *ctx, int argc, term argv[])
{
  (void) argc;

  ErlNifEnv *env = erl_nif_env_from_context(ctx);
  zxp_session_resource_t *resource = NULL;
  if (!enif_get_resource(env, argv[0], zxp_session_resource_type, (void **) &resource) ||
      !term_is_binary(argv[1]) || !term_is_binary(argv[2])) {
    RAISE_ERROR(BADARG_ATOM);
  }

  zxp_session_put_options_t put_options;
  if (!zxp_session_put_options_init(argv[3], &put_options)) {
    zxp_session_put_options_drop(&put_options);
    RAISE_ERROR(BADARG_ATOM);
  }

  z_owned_keyexpr_t keyexpr;
  z_internal_null(&keyexpr);
  z_result_t result = z_keyexpr_from_substr(
      &keyexpr, term_binary_data(argv[1]), term_binary_size(argv[1]));
  if (result != Z_OK) {
    zxp_session_put_options_drop(&put_options);
    return zxp_session_error_tuple(ctx, "z_keyexpr_from_substr");
  }

  z_owned_bytes_t payload;
  z_internal_null(&payload);
  result = z_bytes_copy_from_buf(
      &payload, (const uint8_t *) term_binary_data(argv[2]), term_binary_size(argv[2]));
  if (result != Z_OK) {
    z_drop(z_move(keyexpr));
    zxp_session_put_options_drop(&put_options);
    return zxp_session_error_tuple(ctx, "z_bytes_copy_from_buf");
  }

  pthread_mutex_lock(&resource->mutex);
  if (!z_internal_session_check(&resource->session)) {
    pthread_mutex_unlock(&resource->mutex);
    z_drop(z_move(payload));
    z_drop(z_move(keyexpr));
    zxp_session_put_options_drop(&put_options);
    if (memory_ensure_free_with_roots(ctx, TUPLE_SIZE(2), argc, argv, MEMORY_CAN_SHRINK) !=
        MEMORY_GC_OK) {
      RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    }
    term response = term_alloc_tuple(2, &ctx->heap);
    term_put_tuple_element(response, 0, ERROR_ATOM);
    term_put_tuple_element(response, 1, session_closed_atom);
    return response;
  }
  result = z_put(z_loan(resource->session), z_loan(keyexpr), z_move(payload),
      &put_options.options);
  pthread_mutex_unlock(&resource->mutex);
  z_drop(z_move(keyexpr));
  zxp_session_put_options_drop(&put_options);
  if (result != Z_OK) {
    return zxp_session_error_tuple(ctx, "z_put");
  }
  return OK_ATOM;
}

term zxp_session_get(Context *ctx, int argc, term argv[])
{
  ErlNifEnv *env = erl_nif_env_from_context(ctx);
  zxp_session_resource_t *resource = NULL;
  if (!enif_get_resource(env, argv[0], zxp_session_resource_type, (void **) &resource) ||
      !term_is_binary(argv[1]) || !term_is_uint64(argv[2])) {
    RAISE_ERROR(BADARG_ATOM);
  }

  zxp_session_get_options_t get_options;
  if (!zxp_session_get_options_init(argv[3], &get_options)) {
    zxp_session_get_options_drop(&get_options);
    RAISE_ERROR(BADARG_ATOM);
  }

  uint64_t timeout_ms = term_to_uint64(argv[2]);
  if (get_options.options.timeout_ms > timeout_ms) {
    get_options.options.timeout_ms = timeout_ms;
  }

  const char *selector = term_binary_data(argv[1]);
  size_t selector_size = term_binary_size(argv[1]);
  const char *query_separator = memchr(selector, '?', selector_size);
  size_t keyexpr_size = query_separator == NULL ? selector_size :
      (size_t) (query_separator - selector);
  size_t parameters_size = query_separator == NULL ? 0 : selector_size - keyexpr_size - 1;
  const char *parameters = query_separator == NULL ? NULL : query_separator + 1;

  z_owned_keyexpr_t keyexpr;
  z_internal_null(&keyexpr);
  if (z_keyexpr_from_substr(&keyexpr, selector, keyexpr_size) != Z_OK) {
    zxp_session_get_options_drop(&get_options);
    return zxp_session_error_tuple(ctx, "z_keyexpr_from_substr");
  }

  zxp_session_get_context_t *context = calloc(1, sizeof(*context));
  if (context == NULL) {
    z_drop(z_move(keyexpr));
    zxp_session_get_options_drop(&get_options);
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }
  if (pthread_mutex_init(&context->mutex, NULL) != 0) {
    free(context);
    z_drop(z_move(keyexpr));
    zxp_session_get_options_drop(&get_options);
    return zxp_session_error_tuple(ctx, "pthread_mutex_init");
  }
  if (pthread_cond_init(&context->complete, NULL) != 0) {
    pthread_mutex_destroy(&context->mutex);
    free(context);
    z_drop(z_move(keyexpr));
    zxp_session_get_options_drop(&get_options);
    return zxp_session_error_tuple(ctx, "pthread_cond_init");
  }
  context->ref_count = 1;

  z_owned_closure_reply_t callback;
  z_internal_null(&callback);
  if (z_closure_reply(&callback, zxp_session_get_reply_cb, zxp_session_get_drop_cb, context) != Z_OK) {
    zxp_session_get_context_release(context);
    z_drop(z_move(keyexpr));
    zxp_session_get_options_drop(&get_options);
    return zxp_session_error_tuple(ctx, "z_closure_reply");
  }
  context->ref_count++;

  struct timespec deadline;
  z_result_t result;
  pthread_mutex_lock(&resource->mutex);
  if (!z_internal_session_check(&resource->session)) {
    pthread_mutex_unlock(&resource->mutex);
    z_drop(z_move(callback));
    zxp_session_get_context_release(context);
    z_drop(z_move(keyexpr));
    zxp_session_get_options_drop(&get_options);
    if (memory_ensure_free_with_roots(ctx, TUPLE_SIZE(2), argc, argv, MEMORY_CAN_SHRINK) !=
        MEMORY_GC_OK) {
      RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    }
    term response = term_alloc_tuple(2, &ctx->heap);
    term_put_tuple_element(response, 0, ERROR_ATOM);
    term_put_tuple_element(response, 1, session_closed_atom);
    return response;
  }
  if (!zxp_session_get_deadline(timeout_ms, &deadline)) {
    pthread_mutex_unlock(&resource->mutex);
    z_drop(z_move(callback));
    zxp_session_get_context_release(context);
    z_drop(z_move(keyexpr));
    zxp_session_get_options_drop(&get_options);
    return zxp_session_error_tuple(ctx, "session_get_deadline");
  }
  result = z_get_with_parameters_substr(z_loan(resource->session), z_loan(keyexpr), parameters,
      parameters_size, z_move(callback), &get_options.options);
  pthread_mutex_unlock(&resource->mutex);
  z_drop(z_move(keyexpr));
  zxp_session_get_options_drop(&get_options);
  if (result != Z_OK) {
    zxp_session_get_context_release(context);
    return zxp_session_error_tuple(ctx, "z_get_with_parameters_substr");
  }

  pthread_mutex_lock(&context->mutex);
  while (!context->is_z_get_complete) {
    int wait_result = zxp_session_get_timedwait(context, &deadline);
    if (wait_result == ETIMEDOUT) break;
    if (wait_result != 0) {
      pthread_mutex_unlock(&context->mutex);
      zxp_session_get_context_release(context);
      return zxp_session_error_tuple(ctx, "pthread_cond_timedwait");
    }
  }

  if (context->has_allocation_error) {
    pthread_mutex_unlock(&context->mutex);
    zxp_session_get_context_release(context);
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }
  if (context->reply_count == 0) {
    pthread_mutex_unlock(&context->mutex);
    zxp_session_get_context_release(context);
    if (memory_ensure_free_with_roots(ctx, TUPLE_SIZE(2), argc, argv, MEMORY_CAN_SHRINK) !=
        MEMORY_GC_OK) {
      RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    }
    term response = term_alloc_tuple(2, &ctx->heap);
    term_put_tuple_element(response, 0, ERROR_ATOM);
    term_put_tuple_element(response, 1, timeout_atom);
    return response;
  }

  size_t heap_size = TUPLE_SIZE(2) + context->reply_count * CONS_SIZE;
  for (size_t index = 0; index < context->reply_count; index++) {
    heap_size += context->replies[index].is_ok ?
        zxp_sample_heap_size(&context->replies[index].value.sample) :
        zxp_reply_error_heap_size(&context->replies[index].value.reply_error);
  }
  if (memory_ensure_free_with_roots(ctx, heap_size, argc, argv, MEMORY_CAN_SHRINK) != MEMORY_GC_OK) {
    pthread_mutex_unlock(&context->mutex);
    zxp_session_get_context_release(context);
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  term replies = term_nil();
  for (size_t index = context->reply_count; index > 0; index--) {
    zxp_session_reply_t *reply = &context->replies[index - 1];
    term reply_term = reply->is_ok ? zxp_sample_to_term(ctx, &reply->value.sample) :
        zxp_reply_error_to_term(ctx, &reply->value.reply_error);
    replies = term_list_prepend(reply_term, replies, &ctx->heap);
  }
  term response = term_alloc_tuple(2, &ctx->heap);
  term_put_tuple_element(response, 0, OK_ATOM);
  term_put_tuple_element(response, 1, replies);
  pthread_mutex_unlock(&context->mutex);
  zxp_session_get_context_release(context);
  return response;
}
