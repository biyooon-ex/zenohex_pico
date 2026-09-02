#include <erl_nif.h>
#include <pthread.h>
#include <stdbool.h>
#include <zenoh-pico.h>

typedef struct zxp_subscriber_context zxp_subscriber_context_t;

typedef struct
{
  pthread_mutex_t mutex;
  z_owned_subscriber_t subscriber;
  bool is_mutex_initialized;
} zxp_subscriber_resource_t;

extern ErlNifResourceType *zxp_subscriber_resource_type;

extern void zxp_subscriber_enif_init_resource_type(ErlNifEnv *env);
extern zxp_subscriber_context_t *zxp_subscriber_context_new(const ErlNifPid *pid);
extern void zxp_subscriber_sample_cb(z_loaned_sample_t *sample, void *arg);
extern void zxp_subscriber_drop_cb(void *arg);
extern ERL_NIF_TERM zxp_subscriber_undeclare(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
