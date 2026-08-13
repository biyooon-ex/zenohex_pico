#include <erl_nif.h>

extern ErlNifResourceType *config_resource_type;
extern void config_enif_init_resource_type(ErlNifEnv *env);
extern ERL_NIF_TERM config_default(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
