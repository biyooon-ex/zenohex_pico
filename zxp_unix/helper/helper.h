#include <erl_nif.h>
#include <stddef.h>
#include <stdint.h>

extern ERL_NIF_TERM zxp_binary_from_bytes(ErlNifEnv *env, const uint8_t *data, size_t length);
