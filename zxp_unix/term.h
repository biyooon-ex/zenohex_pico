#include <erl_nif.h>
#include <zenoh-pico.h>

extern ERL_NIF_TERM ok_atom;
extern ERL_NIF_TERM error_atom;

extern void init_atom(ErlNifEnv *env);
extern ERL_NIF_TERM raise(ErlNifEnv *env, const char *file, int line, const char *reason);
extern ERL_NIF_TERM raise_null_pointer(ErlNifEnv *env, const char *file, int line);
extern ERL_NIF_TERM error_tuple_zp(ErlNifEnv *env, const char *file, int line, z_result_t ret);
