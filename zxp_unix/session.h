#include <erl_nif.h>

extern ErlNifResourceType *session_resource_type;
extern ERL_NIF_TERM session_open(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
extern void session_enif_init_resource_type(ErlNifEnv *env);
