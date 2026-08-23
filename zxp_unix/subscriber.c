#include <erl_nif.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <zenoh-pico.h>

#include "macro.h"
#include "sample.h"
#include "subscriber.h"
#include "term.h"

#define ZXP_SUBSCRIBER_QUEUE_CAPACITY 256

typedef enum
{
  ZXP_SUBSCRIBER_QUEUE_FIFO,
  ZXP_SUBSCRIBER_QUEUE_RING,
} zxp_subscriber_queue_policy_t;

struct zxp_subscriber_context
{
  pthread_mutex_t mutex;
  pthread_cond_t not_empty;
  pthread_cond_t not_full;
  pthread_t forwarder;
  ErlNifPid pid;
  z_owned_sample_t queue[ZXP_SUBSCRIBER_QUEUE_CAPACITY];
  size_t head;
  size_t tail;
  size_t count;
  bool stopping;
  bool forwarder_started;
  zxp_subscriber_queue_policy_t queue_policy;
};

ErlNifResourceType *zxp_subscriber_resource_type = NULL;

static void zxp_subscriber_context_clear(zxp_subscriber_context_t *context)
{
  for (size_t index = 0; index < ZXP_SUBSCRIBER_QUEUE_CAPACITY; index++)
  {
    z_drop(z_move(context->queue[index]));
  }
  pthread_cond_destroy(&context->not_full);
  pthread_cond_destroy(&context->not_empty);
  pthread_mutex_destroy(&context->mutex);
  enif_free(context);
}

static void *zxp_subscriber_forwarder(void *arg)
{
  zxp_subscriber_context_t *context = arg;
  ErlNifEnv *env = enif_alloc_env();
  if (env == NULL)
  {
    return NULL;
  }

  for (;;)
  {
    z_owned_sample_t sample;
    z_internal_null(&sample);

    pthread_mutex_lock(&context->mutex);
    while (context->count == 0 && !context->stopping)
    {
      pthread_cond_wait(&context->not_empty, &context->mutex);
    }
    if (context->stopping)
    {
      pthread_mutex_unlock(&context->mutex);
      z_drop(z_move(sample));
      break;
    }

    z_take(&sample, z_move(context->queue[context->head]));
    context->head = (context->head + 1) % ZXP_SUBSCRIBER_QUEUE_CAPACITY;
    context->count--;
    pthread_cond_signal(&context->not_full);
    pthread_mutex_unlock(&context->mutex);

    ERL_NIF_TERM term = zxp_struct_from_zp_sample(env, z_loan(sample));
    if (!enif_is_exception(env, term))
    {
      enif_send(NULL, &context->pid, env, term);
    }
    enif_clear_env(env);
    z_drop(z_move(sample));
  }

  enif_free_env(env);
  return NULL;
}

void zxp_subscriber_sample_cb(z_loaned_sample_t *sample, void *arg)
{
  zxp_subscriber_context_t *context = arg;
  z_owned_sample_t copy;
  z_internal_null(&copy);
  if (z_clone(&copy, sample) != Z_OK)
  {
    return;
  }

  pthread_mutex_lock(&context->mutex);
  while (context->count == ZXP_SUBSCRIBER_QUEUE_CAPACITY && !context->stopping)
  {
    if (context->queue_policy == ZXP_SUBSCRIBER_QUEUE_RING)
    {
      z_drop(z_move(context->queue[context->head]));
      context->head = (context->head + 1) % ZXP_SUBSCRIBER_QUEUE_CAPACITY;
      context->count--;
      break;
    }
    pthread_cond_wait(&context->not_full, &context->mutex);
  }

  if (context->stopping)
  {
    pthread_mutex_unlock(&context->mutex);
    z_drop(z_move(copy));
    return;
  }

  z_take(&context->queue[context->tail], z_move(copy));
  context->tail = (context->tail + 1) % ZXP_SUBSCRIBER_QUEUE_CAPACITY;
  context->count++;
  pthread_cond_signal(&context->not_empty);
  pthread_mutex_unlock(&context->mutex);
}

void zxp_subscriber_drop_cb(void *arg) { UNUSED(arg); }

bool zxp_subscriber_init(zxp_subscriber_t *subscriber, const ErlNifPid *pid)
{
  zxp_subscriber_context_t *context = enif_alloc(sizeof(*context));
  if (context == NULL)
  {
    return false;
  }

  *context = (zxp_subscriber_context_t){
      .pid = *pid,
      .queue_policy = ZXP_SUBSCRIBER_QUEUE_FIFO,
  };
  for (size_t index = 0; index < ZXP_SUBSCRIBER_QUEUE_CAPACITY; index++)
  {
    z_internal_null(&context->queue[index]);
  }
  if (pthread_mutex_init(&context->mutex, NULL) != 0)
  {
    enif_free(context);
    return false;
  }
  if (pthread_cond_init(&context->not_empty, NULL) != 0)
  {
    pthread_mutex_destroy(&context->mutex);
    enif_free(context);
    return false;
  }
  if (pthread_cond_init(&context->not_full, NULL) != 0)
  {
    pthread_cond_destroy(&context->not_empty);
    pthread_mutex_destroy(&context->mutex);
    enif_free(context);
    return false;
  }
  if (pthread_create(&context->forwarder, NULL, zxp_subscriber_forwarder, context) != 0)
  {
    zxp_subscriber_context_clear(context);
    return false;
  }

  context->forwarder_started = true;
  z_internal_null(&subscriber->subscriber);
  subscriber->context = context;
  subscriber->undeclared = false;
  return true;
}

z_result_t zxp_subscriber_shutdown(zxp_subscriber_t *subscriber, bool undeclare)
{
  zxp_subscriber_context_t *context = subscriber->context;
  if (context == NULL)
  {
    return Z_OK;
  }

  pthread_mutex_lock(&context->mutex);
  context->stopping = true;
  pthread_cond_broadcast(&context->not_empty);
  pthread_cond_broadcast(&context->not_full);
  pthread_mutex_unlock(&context->mutex);

  if (!subscriber->undeclared)
  {
    if (undeclare)
    {
      z_result_t result = z_undeclare_subscriber(z_move(subscriber->subscriber));
      subscriber->undeclared = true;
      if (context->forwarder_started)
      {
        pthread_join(context->forwarder, NULL);
      }
      zxp_subscriber_context_clear(context);
      subscriber->context = NULL;
      return result;
    }
    else
    {
      z_drop(z_move(subscriber->subscriber));
    }
    subscriber->undeclared = true;
  }
  if (context->forwarder_started)
  {
    pthread_join(context->forwarder, NULL);
  }
  zxp_subscriber_context_clear(context);
  subscriber->context = NULL;
  return Z_OK;
}

static void zxp_subscriber_dtor(ErlNifEnv *env, void *obj)
{
  UNUSED(env);
  (void)zxp_subscriber_shutdown((zxp_subscriber_t *)obj, false);
}

static const ErlNifResourceTypeInit ZxpSubscriberResourceTypeInit = {
    .dtor = zxp_subscriber_dtor,
    .members = 1,
};

void zxp_subscriber_enif_init_resource_type(ErlNifEnv *env)
{
  zxp_subscriber_resource_type = enif_init_resource_type(
      env, "zxp_subscriber", &ZxpSubscriberResourceTypeInit, ERL_NIF_RT_CREATE, NULL);
}

ERL_NIF_TERM zxp_subscriber_undeclare(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  UNUSED(argc);

  zxp_subscriber_t *subscriber = NULL;
  if (!enif_get_resource(env, argv[0], zxp_subscriber_resource_type, (void **)&subscriber))
  {
    return enif_make_badarg(env);
  }
  if (subscriber->undeclared)
  {
    return enif_make_tuple2(env, error_atom, not_found_atom);
  }

  z_result_t result = zxp_subscriber_shutdown(subscriber, true);
  if (result != Z_OK)
  {
    return zxp_error_tuple_zp(env, __FILE__, __LINE__, result);
  }
  return ok_atom;
}
