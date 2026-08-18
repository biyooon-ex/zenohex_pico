#include <erl_nif.h>
#include <stdint.h>

ERL_NIF_TERM zxp_binary_from_bytes(ErlNifEnv *env, const uint8_t *data, size_t length)
{
  ERL_NIF_TERM binary;
  uint8_t *destination = enif_make_new_binary(env, length, &binary);
  if (length > 0)
  {
    memcpy(destination, data, length);
  }
  return binary;
}
