#include <erl_nif.h>

extern ErlNifResourceType *zxp_session_resource_type;
extern ERL_NIF_TERM zxp_session_open(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
extern void zxp_session_enif_init_resource_type(ErlNifEnv *env);
