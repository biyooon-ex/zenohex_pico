#include <erl_nif.h>
#include <stdbool.h>
#include <zenoh-pico.h>

typedef struct zxp_subscriber_context zxp_subscriber_context_t;

typedef struct
{
  z_owned_subscriber_t subscriber;
  zxp_subscriber_context_t *context;
  bool undeclared;
} zxp_subscriber_t;

extern ErlNifResourceType *zxp_subscriber_resource_type;

extern void zxp_subscriber_enif_init_resource_type(ErlNifEnv *env);
extern bool zxp_subscriber_init(zxp_subscriber_t *subscriber, const ErlNifPid *pid);
extern z_result_t zxp_subscriber_shutdown(zxp_subscriber_t *subscriber, bool undeclare);
extern void zxp_subscriber_sample_cb(z_loaned_sample_t *sample, void *arg);
extern void zxp_subscriber_drop_cb(void *arg);
extern ERL_NIF_TERM zxp_subscriber_undeclare(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
