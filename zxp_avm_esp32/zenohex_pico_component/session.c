#include <defaultatoms.h>
#include <erl_nif_priv.h>
#include <errno.h>
#include <memory.h>
#include <nifs.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <term.h>
#include <time.h>
#include <utils.h>
#include <zenoh-pico.h>

#include "config.h"
#include "query.h"
#include "sample.h"
#include "session.h"
#include "session_option.h"
#include "time_compat.h"
#include "zxp_term.h"

#define ZXP_SESSION_GET_INITIAL_REPLY_CAPACITY 4

ErlNifResourceType *zxp_session_resource_type = NULL;

typedef struct
{
  pthread_mutex_t mutex;
  z_owned_session_t session;
  bool is_mutex_initialized;
} zxp_session_resource_t;

typedef struct
{
  pthread_mutex_t mutex;
  pthread_cond_t complete;
  bool is_z_get_complete;
  bool has_allocation_error;
  size_t ref_count;
  size_t reply_count;
  size_t reply_capacity;
  z_owned_reply_t *replies;
} zxp_session_get_context_t;

static void zxp_session_get_context_release(zxp_session_get_context_t *context)
{
  bool should_free = false;

  pthread_mutex_lock(&context->mutex);
  {
    context->ref_count--;
    should_free = context->ref_count == 0;
  }
  pthread_mutex_unlock(&context->mutex);

  if (should_free)
  {
    for (size_t index = 0; index < context->reply_count; index++)
    {
      z_drop(z_move(context->replies[index]));
    }
    free(context->replies);
    pthread_cond_destroy(&context->complete);
    pthread_mutex_destroy(&context->mutex);
    free(context);
  }
}

static void zxp_session_get_reply_cb(z_loaned_reply_t *reply, void *arg)
{
  zxp_session_get_context_t *context = arg;

  pthread_mutex_lock(&context->mutex);
  {
    if (context->has_allocation_error)
    {
      pthread_mutex_unlock(&context->mutex);
      return;
    }

    if (context->reply_count == context->reply_capacity)
    {
      size_t capacity = context->reply_capacity == 0 ? ZXP_SESSION_GET_INITIAL_REPLY_CAPACITY
                                                     : context->reply_capacity * 2;
      z_owned_reply_t *replies = realloc(context->replies, capacity * sizeof(*replies));
      if (replies == NULL)
      {
        context->has_allocation_error = true;
        pthread_mutex_unlock(&context->mutex);
        return;
      }
      context->replies = replies;
      context->reply_capacity = capacity;
    }

    z_owned_reply_t *destination = &context->replies[context->reply_count];
    z_internal_null(destination);
    if (z_clone(destination, reply) != Z_OK)
    {
      context->has_allocation_error = true;
    }
    else
    {
      context->reply_count++;
    }
  }
  pthread_mutex_unlock(&context->mutex);
}

static void zxp_session_get_drop_cb(void *arg)
{
  zxp_session_get_context_t *context = arg;
  pthread_mutex_lock(&context->mutex);
  {
    context->is_z_get_complete = true;
    pthread_cond_signal(&context->complete);
  }
  pthread_mutex_unlock(&context->mutex);
  zxp_session_get_context_release(context);
}

static bool zxp_session_get_deadline(uint64_t timeout_ms, struct timespec *deadline)
{
  if (clock_gettime(CLOCK_MONOTONIC, deadline) != 0 || deadline->tv_sec < 0)
  {
    return false;
  }

  uint64_t timeout_seconds = timeout_ms / 1000;
  long deadline_nanoseconds = deadline->tv_nsec + (long)((timeout_ms % 1000) * 1000000);
  if (deadline_nanoseconds >= 1000000000L)
  {
    timeout_seconds++;
    deadline_nanoseconds -= 1000000000L;
  }

  uint64_t current_seconds = (uint64_t)deadline->tv_sec;
  if ((time_t)current_seconds != deadline->tv_sec || timeout_seconds > UINT64_MAX - current_seconds)
  {
    return false;
  }

  uint64_t deadline_seconds = current_seconds + timeout_seconds;
  time_t converted_deadline_seconds = (time_t)deadline_seconds;
  if ((uint64_t)converted_deadline_seconds != deadline_seconds)
  {
    return false;
  }

  deadline->tv_sec = converted_deadline_seconds;
  deadline->tv_nsec = deadline_nanoseconds;
  return true;
}

static void zxp_session_dtor(ErlNifEnv *env, void *obj)
{
  UNUSED(env);

  zxp_session_resource_t *resource = obj;
  if (!resource->is_mutex_initialized)
  {
    return;
  }

  z_owned_session_t session;
  z_internal_null(&session);

  pthread_mutex_lock(&resource->mutex);
  {
    if (z_internal_session_check(&resource->session))
    {
      z_take(&session, z_move(resource->session));
    }
  }
  pthread_mutex_unlock(&resource->mutex);

  if (z_internal_session_check(&session))
  {
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
  return zxp_session_resource_type != NULL;
}

term zxp_session_open(Context *ctx, int argc, term argv[])
{
  UNUSED(argc);

  ErlNifEnv *env = erl_nif_env_from_context(ctx);
  z_owned_config_t *config_p = NULL;
  if (!enif_get_resource(env, argv[0], zxp_config_resource_type, (void **)&config_p))
  {
    RAISE_ERROR(BADARG_ATOM);
  }

  z_owned_config_t config;
  z_internal_null(&config);
  {
    z_result_t ret = z_clone(&config, z_loan(*config_p));

    if (ret != Z_OK)
    {
      return zxp_error_tuple_zp(ctx, ret);
    }
  }

  z_owned_session_t session;
  z_internal_null(&session);
  {
    z_result_t ret = z_open(&session, z_move(config), NULL);

    if (ret != Z_OK)
    {
      return zxp_error_tuple_zp(ctx, ret);
    }
  }

  zxp_session_resource_t *resource =
      enif_alloc_resource(zxp_session_resource_type, sizeof(*resource));
  if (resource == NULL)
  {
    z_drop(z_move(session));
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  z_internal_null(&resource->session);
  resource->is_mutex_initialized = false;
  if (pthread_mutex_init(&resource->mutex, NULL) != 0)
  {
    z_drop(z_move(session));
    enif_release_resource(resource);
    return zxp_raise(ctx, "pthread_mutex_init/2 failed");
  }

  resource->is_mutex_initialized = true;
  z_take(&resource->session, z_move(session));
  if (memory_ensure_free(ctx, TERM_BOXED_RESOURCE_SIZE + TUPLE_SIZE(2)) != MEMORY_GC_OK)
  {
    enif_release_resource(resource);
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  term session_ref = term_from_resource(resource, &ctx->heap);
  enif_release_resource(resource);

  return zxp_make_tuple2(ctx, OK_ATOM, session_ref);
}

term zxp_session_close(Context *ctx, int argc, term argv[])
{
  UNUSED(argc);

  ErlNifEnv *env = erl_nif_env_from_context(ctx);
  zxp_session_resource_t *resource = NULL;
  if (!enif_get_resource(env, argv[0], zxp_session_resource_type, (void **)&resource))
  {
    RAISE_ERROR(BADARG_ATOM);
  }

  z_owned_session_t session;
  z_internal_null(&session);

  pthread_mutex_lock(&resource->mutex);
  {
    if (!z_internal_session_check(&resource->session))
    {
      pthread_mutex_unlock(&resource->mutex);
      if (memory_ensure_free(ctx, TUPLE_SIZE(2)) != MEMORY_GC_OK)
      {
        RAISE_ERROR(OUT_OF_MEMORY_ATOM);
      }
      return zxp_make_tuple2(ctx, ERROR_ATOM, session_closed_atom);
    }

    z_take(&session, z_move(resource->session));
  }
  pthread_mutex_unlock(&resource->mutex);

  // Release session-owned transports now because the NIF resource destructor is GC-driven.
  z_drop(z_move(session));

  return OK_ATOM;
}

term zxp_session_put(Context *ctx, int argc, term argv[])
{
  UNUSED(argc);

  ErlNifEnv *env = erl_nif_env_from_context(ctx);
  zxp_session_resource_t *resource = NULL;
  if (!enif_get_resource(env, argv[0], zxp_session_resource_type, (void **)&resource))
  {
    RAISE_ERROR(BADARG_ATOM);
  }

  if (!term_is_binary(argv[1]))
  {
    RAISE_ERROR(BADARG_ATOM);
  }

  if (!term_is_binary(argv[2]))
  {
    RAISE_ERROR(BADARG_ATOM);
  }

  zxp_session_put_options_t *put_options = NULL;
  term error;
  if (!zxp_session_put_options_new(ctx, &put_options, &error))
  {
    return error;
  }

  if (!zxp_session_put_options_init(ctx, argv[3], put_options, &error))
  {
    zxp_session_put_options_drop(put_options);
    if (error == BADARG_ATOM)
    {
      RAISE_ERROR(BADARG_ATOM);
    }
    return error;
  }

  z_owned_keyexpr_t keyexpr;
  z_internal_null(&keyexpr);
  z_result_t ret =
      z_keyexpr_from_substr(&keyexpr, term_binary_data(argv[1]), term_binary_size(argv[1]));
  if (ret != Z_OK)
  {
    zxp_session_put_options_drop(put_options);
    return zxp_error_tuple_zp(ctx, ret);
  }

  z_owned_bytes_t payload;
  z_internal_null(&payload);
  ret = z_bytes_copy_from_buf(
      &payload, (const uint8_t *)term_binary_data(argv[2]), term_binary_size(argv[2]));
  if (ret != Z_OK)
  {
    z_drop(z_move(keyexpr));
    zxp_session_put_options_drop(put_options);
    return zxp_error_tuple_zp(ctx, ret);
  }

  pthread_mutex_lock(&resource->mutex);
  {
    if (!z_internal_session_check(&resource->session))
    {
      pthread_mutex_unlock(&resource->mutex);
      z_drop(z_move(payload));
      z_drop(z_move(keyexpr));
      zxp_session_put_options_drop(put_options);
      if (memory_ensure_free(ctx, TUPLE_SIZE(2)) != MEMORY_GC_OK)
      {
        RAISE_ERROR(OUT_OF_MEMORY_ATOM);
      }
      return zxp_make_tuple2(ctx, ERROR_ATOM, session_closed_atom);
    }

    ret = z_put(z_loan(resource->session),
                z_loan(keyexpr),
                z_move(payload),
                zxp_session_put_options_loan(put_options));
  }
  pthread_mutex_unlock(&resource->mutex);
  z_drop(z_move(keyexpr));
  zxp_session_put_options_drop(put_options);
  if (ret != Z_OK)
  {
    return zxp_error_tuple_zp(ctx, ret);
  }

  return OK_ATOM;
}

term zxp_session_get(Context *ctx, int argc, term argv[])
{
  ErlNifEnv *env = erl_nif_env_from_context(ctx);
  zxp_session_resource_t *resource = NULL;
  if (!enif_get_resource(env, argv[0], zxp_session_resource_type, (void **)&resource))
  {
    RAISE_ERROR(BADARG_ATOM);
  }

  if (!term_is_binary(argv[1]))
  {
    RAISE_ERROR(BADARG_ATOM);
  }
  const char *selector = term_binary_data(argv[1]);
  size_t selector_length = term_binary_size(argv[1]);

  if (!term_is_uint64(argv[2]))
  {
    RAISE_ERROR(BADARG_ATOM);
  }
  uint64_t timeout_ms = term_to_uint64(argv[2]);

  zxp_session_get_options_t *get_options = NULL;
  term error;
  if (!zxp_session_get_options_new(ctx, &get_options, &error))
  {
    return error;
  }

  if (!zxp_session_get_options_init(ctx, argv[3], get_options, &error))
  {
    zxp_session_get_options_drop(get_options);
    if (error == BADARG_ATOM)
    {
      RAISE_ERROR(BADARG_ATOM);
    }
    return error;
  }

  // Do not let Zenoh retain the query context after this NIF has stopped waiting for replies.
  if (get_options->options.timeout_ms > timeout_ms)
  {
    get_options->options.timeout_ms = timeout_ms;
  }

  const char *query_separator = memchr(selector, '?', selector_length);
  size_t keyexpr_length =
      query_separator == NULL ? selector_length : (size_t)(query_separator - selector);
  size_t parameters_length = query_separator == NULL ? 0 : selector_length - keyexpr_length - 1;

  z_owned_keyexpr_t keyexpr;
  z_internal_null(&keyexpr);
  z_result_t ret = z_keyexpr_from_substr(&keyexpr, selector, keyexpr_length);
  if (ret != Z_OK)
  {
    zxp_session_get_options_drop(get_options);
    return zxp_error_tuple_zp(ctx, ret);
  }

  const char *parameters = query_separator == NULL ? NULL : query_separator + 1;

  zxp_session_get_context_t *context = malloc(sizeof(*context));
  if (context == NULL)
  {
    z_drop(z_move(keyexpr));
    zxp_session_get_options_drop(get_options);
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  memset(context, 0, sizeof(*context));
  if (pthread_mutex_init(&context->mutex, NULL) != 0)
  {
    free(context);
    z_drop(z_move(keyexpr));
    zxp_session_get_options_drop(get_options);
    return zxp_raise(ctx, "pthread_mutex_init/2 failed");
  }

  if (pthread_cond_init(&context->complete, NULL) != 0)
  {
    pthread_mutex_destroy(&context->mutex);
    free(context);
    z_drop(z_move(keyexpr));
    zxp_session_get_options_drop(get_options);
    return zxp_raise(ctx, "pthread_cond_init/2 failed");
  }
  context->ref_count = 1;

  z_owned_closure_reply_t callback;
  z_internal_null(&callback);
  ret = z_closure_reply(&callback, zxp_session_get_reply_cb, zxp_session_get_drop_cb, context);
  if (ret != Z_OK)
  {
    zxp_session_get_context_release(context);
    z_drop(z_move(keyexpr));
    zxp_session_get_options_drop(get_options);
    return zxp_error_tuple_zp(ctx, ret);
  }
  context->ref_count++;

  struct timespec deadline;
  pthread_mutex_lock(&resource->mutex);
  {
    if (!z_internal_session_check(&resource->session))
    {
      pthread_mutex_unlock(&resource->mutex);
      z_drop(z_move(callback));
      zxp_session_get_context_release(context);
      z_drop(z_move(keyexpr));
      zxp_session_get_options_drop(get_options);
      if (memory_ensure_free(ctx, TUPLE_SIZE(2)) != MEMORY_GC_OK)
      {
        RAISE_ERROR(OUT_OF_MEMORY_ATOM);
      }
      return zxp_make_tuple2(ctx, ERROR_ATOM, session_closed_atom);
    }
    // Start the reply-wait timeout after option parsing, allocation, and session-lock acquisition.
    // Calculate it before issuing the query so a clock failure cannot leave a query running.
    if (!zxp_session_get_deadline(timeout_ms, &deadline))
    {
      pthread_mutex_unlock(&resource->mutex);
      z_drop(z_move(callback));
      zxp_session_get_context_release(context);
      z_drop(z_move(keyexpr));
      zxp_session_get_options_drop(get_options);
      return zxp_raise(ctx, "failed to calculate session get deadline");
    }

    ret = z_get_with_parameters_substr(z_loan(resource->session),
                                       z_loan(keyexpr),
                                       parameters,
                                       parameters_length,
                                       z_move(callback),
                                       zxp_session_get_options_loan(get_options));
  }
  pthread_mutex_unlock(&resource->mutex);
  z_drop(z_move(keyexpr));
  zxp_session_get_options_drop(get_options);
  if (ret != Z_OK)
  {
    zxp_session_get_context_release(context);
    return zxp_error_tuple_zp(ctx, ret);
  }

  term replies;
  pthread_mutex_lock(&context->mutex);
  {
    while (!context->is_z_get_complete)
    {
      struct timespec realtime_deadline;
      int conversion_result = zxp_monotonic_to_realtime_deadline(&deadline, &realtime_deadline);
      if (conversion_result == ETIMEDOUT)
      {
        break;
      }
      if (conversion_result != 0)
      {
        pthread_mutex_unlock(&context->mutex);
        zxp_session_get_context_release(context);
        return zxp_raise(ctx, "zxp_monotonic_to_realtime_deadline");
      }

      int wait_result =
          pthread_cond_timedwait(&context->complete, &context->mutex, &realtime_deadline);
      if (wait_result == ETIMEDOUT)
      {
        break;
      }
      if (wait_result != 0)
      {
        pthread_mutex_unlock(&context->mutex);
        zxp_session_get_context_release(context);
        return zxp_raise(ctx, "pthread_cond_timedwait");
      }
    }

    if (context->has_allocation_error)
    {
      pthread_mutex_unlock(&context->mutex);
      zxp_session_get_context_release(context);
      RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    }

    if (context->reply_count == 0)
    {
      pthread_mutex_unlock(&context->mutex);
      zxp_session_get_context_release(context);
      if (memory_ensure_free(ctx, TUPLE_SIZE(2)) != MEMORY_GC_OK)
      {
        RAISE_ERROR(OUT_OF_MEMORY_ATOM);
      }
      return zxp_make_tuple2(ctx, ERROR_ATOM, timeout_atom);
    }

    size_t heap_size = TUPLE_SIZE(2) + context->reply_count * CONS_SIZE;
    for (size_t index = 0; index < context->reply_count; index++)
    {
      const z_loaned_reply_t *reply = z_loan(context->replies[index]);
      if (z_reply_is_ok(reply))
      {
        size_t sample_heap_size;
        if (!zxp_sample_heap_size(z_reply_ok(reply), &sample_heap_size))
        {
          pthread_mutex_unlock(&context->mutex);
          zxp_session_get_context_release(context);
          RAISE_ERROR(OUT_OF_MEMORY_ATOM);
        }
        heap_size += sample_heap_size;
      }
      else
      {
        size_t reply_error_heap_size;
        if (!zxp_reply_error_heap_size(z_reply_err(reply), &reply_error_heap_size))
        {
          pthread_mutex_unlock(&context->mutex);
          zxp_session_get_context_release(context);
          RAISE_ERROR(OUT_OF_MEMORY_ATOM);
        }
        heap_size += reply_error_heap_size;
      }
    }
    if (memory_ensure_free(ctx, heap_size) != MEMORY_GC_OK)
    {
      pthread_mutex_unlock(&context->mutex);
      zxp_session_get_context_release(context);
      RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    }

    replies = zxp_avm_empty_list();
    for (size_t index = context->reply_count; index > 0; index--)
    {
      const z_loaned_reply_t *loaned_reply = z_loan(context->replies[index - 1]);
      term reply_term = z_reply_is_ok(loaned_reply)
                            ? zxp_struct_from_zp_sample(ctx, z_reply_ok(loaned_reply))
                            : zxp_struct_from_zp_reply_err(ctx, z_reply_err(loaned_reply));
      if (term_is_invalid_term(reply_term))
      {
        pthread_mutex_unlock(&context->mutex);
        zxp_session_get_context_release(context);
        return term_invalid_term();
      }
      replies = term_list_prepend(reply_term, replies, &ctx->heap);
    }
  }
  pthread_mutex_unlock(&context->mutex);
  zxp_session_get_context_release(context);

  return zxp_make_tuple2(ctx, OK_ATOM, replies);
}
