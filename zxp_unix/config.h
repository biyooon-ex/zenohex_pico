#include <erl_nif.h>

extern ErlNifResourceType *zxp_config_resource_type;
extern void zxp_config_enif_init_resource_type(ErlNifEnv *env);
extern ERL_NIF_TERM zxp_config_default(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
extern ERL_NIF_TERM zxp_config_get(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
extern ERL_NIF_TERM zxp_config_insert(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
