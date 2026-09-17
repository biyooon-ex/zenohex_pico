#include <erl_nif.h>
#include <stdbool.h>
#include <zenoh-pico.h>

extern bool zxp_timestamp_from_binary(const ErlNifBinary *binary, z_timestamp_t *timestamp);
extern ERL_NIF_TERM zxp_binary_from_zp_timestamp(ErlNifEnv *env, const z_timestamp_t *timestamp);
