#include <erl_nif.h>
#include <zenoh-pico.h>

extern ERL_NIF_TERM ok_atom;
extern ERL_NIF_TERM error_atom;
extern ERL_NIF_TERM not_found_atom;

extern void zxp_init_atom(ErlNifEnv *env);
extern ERL_NIF_TERM zxp_raise(ErlNifEnv *env, const char *file, int line, const char *reason);
extern ERL_NIF_TERM zxp_raise_null_pointer(ErlNifEnv *env, const char *file, int line);
extern ERL_NIF_TERM zxp_error_tuple_zp(ErlNifEnv *env, const char *file, int line, z_result_t ret);
extern ERL_NIF_TERM zxp_test_raise(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
