#include <erl_nif.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <zenoh-pico.h>
#include <zenoh-pico/utils/result.h>

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
  size_t dequeue_index;
  size_t enqueue_index;
  size_t queued_count;
  bool stopping;
  bool forwarder_started;
  zxp_subscriber_queue_policy_t queue_policy;
};

ErlNifResourceType *zxp_subscriber_resource_type = NULL;

static void zxp_subscriber_context_release(zxp_subscriber_context_t *context)
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
    pthread_mutex_lock(&context->mutex);
    context->stopping = true;
    pthread_cond_broadcast(&context->not_full);
    pthread_mutex_unlock(&context->mutex);
    return NULL;
  }

  for (;;)
  {
    z_owned_sample_t sample;
    z_internal_null(&sample);

    pthread_mutex_lock(&context->mutex);
    while (context->queued_count == 0 && !context->stopping)
    {
      pthread_cond_wait(&context->not_empty, &context->mutex);
    }
    if (context->stopping)
    {
      pthread_mutex_unlock(&context->mutex);
      z_drop(z_move(sample));
      break;
    }

    z_take(&sample, z_move(context->queue[context->dequeue_index]));
    context->dequeue_index = (context->dequeue_index + 1) % ZXP_SUBSCRIBER_QUEUE_CAPACITY;
    context->queued_count--;
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
  if (context->queued_count == ZXP_SUBSCRIBER_QUEUE_CAPACITY)
  {
    switch (context->queue_policy)
    {
    case ZXP_SUBSCRIBER_QUEUE_FIFO:
      while (context->queued_count == ZXP_SUBSCRIBER_QUEUE_CAPACITY && !context->stopping)
      {
        pthread_cond_wait(&context->not_full, &context->mutex);
      }
      break;
    case ZXP_SUBSCRIBER_QUEUE_RING:
      z_drop(z_move(context->queue[context->dequeue_index]));
      context->dequeue_index = (context->dequeue_index + 1) % ZXP_SUBSCRIBER_QUEUE_CAPACITY;
      context->queued_count--;
      break;
    }
  }

  if (context->stopping)
  {
    pthread_mutex_unlock(&context->mutex);
    z_drop(z_move(copy));
    return;
  }

  z_take(&context->queue[context->enqueue_index], z_move(copy));
  context->enqueue_index = (context->enqueue_index + 1) % ZXP_SUBSCRIBER_QUEUE_CAPACITY;
  context->queued_count++;
  pthread_cond_signal(&context->not_empty);
  pthread_mutex_unlock(&context->mutex);
}

zxp_subscriber_context_t *zxp_subscriber_context_new(const ErlNifPid *pid)
{
  zxp_subscriber_context_t *context = enif_alloc(sizeof(*context));
  if (context == NULL)
  {
    return NULL;
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
    return NULL;
  }
  if (pthread_cond_init(&context->not_empty, NULL) != 0)
  {
    pthread_mutex_destroy(&context->mutex);
    enif_free(context);
    return NULL;
  }
  if (pthread_cond_init(&context->not_full, NULL) != 0)
  {
    pthread_cond_destroy(&context->not_empty);
    pthread_mutex_destroy(&context->mutex);
    enif_free(context);
    return NULL;
  }
  if (pthread_create(&context->forwarder, NULL, zxp_subscriber_forwarder, context) != 0)
  {
    zxp_subscriber_context_release(context);
    return NULL;
  }

  context->forwarder_started = true;
  return context;
}

void zxp_subscriber_drop_cb(void *arg)
{
  zxp_subscriber_context_t *context = arg;

  pthread_mutex_lock(&context->mutex);
  context->stopping = true;
  pthread_cond_broadcast(&context->not_empty);
  pthread_cond_broadcast(&context->not_full);
  pthread_mutex_unlock(&context->mutex);

  if (context->forwarder_started)
  {
    pthread_join(context->forwarder, NULL);
  }
  zxp_subscriber_context_release(context);
}

static void zxp_subscriber_dtor(ErlNifEnv *env, void *obj)
{
  UNUSED(env);

  zxp_subscriber_resource_t *resource = obj;
  if (!resource->is_mutex_initialized)
  {
    return;
  }

  z_owned_subscriber_t subscriber;
  z_internal_null(&subscriber);

  pthread_mutex_lock(&resource->mutex);
  {
    if (z_internal_subscriber_check(&resource->subscriber))
    {
      z_take(&subscriber, z_move(resource->subscriber));
      z_internal_null(&resource->subscriber);
    }
  }
  pthread_mutex_unlock(&resource->mutex);

  if (z_internal_subscriber_check(&subscriber))
  {
    z_drop(z_move(subscriber));
  }
  pthread_mutex_destroy(&resource->mutex);
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

  zxp_subscriber_resource_t *resource = NULL;
  if (!enif_get_resource(env, argv[0], zxp_subscriber_resource_type, (void **)&resource))
  {
    return enif_make_badarg(env);
  }

  z_owned_subscriber_t subscriber;
  z_internal_null(&subscriber);

  pthread_mutex_lock(&resource->mutex);
  {
    if (!z_internal_subscriber_check(&resource->subscriber))
    {
      pthread_mutex_unlock(&resource->mutex);
      return enif_make_tuple2(env, error_atom, session_closed_atom);
    }

    z_take(&subscriber, z_move(resource->subscriber));
    z_internal_null(&resource->subscriber);
  }
  pthread_mutex_unlock(&resource->mutex);

  z_result_t ret = z_undeclare_subscriber(z_move(subscriber));
  if (ret != Z_OK)
  {
    return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
  }

  return ok_atom;
}
