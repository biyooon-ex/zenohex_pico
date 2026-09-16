#include <erl_nif.h>
#include <stdint.h>
#include <string.h>

#include "../zxp_term.h"

ERL_NIF_TERM zxp_binary_from_bytes(ErlNifEnv *env, const uint8_t *data, size_t length)
{
  ERL_NIF_TERM binary;
  uint8_t *dest = enif_make_new_binary(env, length, &binary);
  if (dest == NULL)
  {
    return zxp_raise_null_pointer(env, __FILE__, __LINE__);
  }
  if (length > 0)
  {
    memcpy(dest, data, length);
  }
  return binary;
}
