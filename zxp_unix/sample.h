#include <erl_nif.h>
#include <zenoh-pico.h>

extern ERL_NIF_TERM zxp_struct_from_zp_sample(ErlNifEnv *env, const z_loaned_sample_t *sample);
