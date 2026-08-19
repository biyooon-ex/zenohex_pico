#include <erl_nif.h>
#include <zenoh-pico.h>

#include "helper/helper.h"
#include "term.h"

typedef struct
{
  uint8_t *payload;
  size_t payload_len;
  char *encoding;
} zxp_reply_error_t;

ERL_NIF_TERM zxp_to_reply_error_term(ErlNifEnv *env, const zxp_reply_error_t *reply)
{

  ERL_NIF_TERM keys[] = {
      struct_atom,
      payload_atom,
      encoding_atom,
  };

  ERL_NIF_TERM values[] = {
      reply_error_module,
      zxp_binary_from_bytes(env, reply->payload, reply->payload_len),
      zxp_binary_from_bytes(env, (const uint8_t *)reply->encoding, strlen(reply->encoding)),
  };

  ERL_NIF_TERM term;
  enif_make_map_from_arrays(env, keys, values, 3, &term);
  return term;
}

static ERL_NIF_TERM zxp_binary_from_zp_bytes(ErlNifEnv *env, const z_loaned_bytes_t *bytes)
{
  if (bytes == NULL)
  {
    return nil_atom;
  }

  z_owned_slice_t slice;
  z_result_t ret = z_bytes_to_slice(bytes, &slice);
  if (ret != Z_OK)
  {
    return zxp_raise(env, __FILE__, __LINE__, zxp_error_char_zp(ret));
  }

  ERL_NIF_TERM term =
      zxp_binary_from_bytes(env, z_slice_data(z_loan(slice)), z_slice_len(z_loan(slice)));
  z_drop(z_move(slice));
  return term;
}

static ERL_NIF_TERM zxp_binary_from_zp_encoding(ErlNifEnv *env, const z_loaned_encoding_t *encoding)
{
  z_owned_string_t string;
  ERL_NIF_TERM term;
  {
    z_result_t ret = z_encoding_to_string(encoding, &string);
    if (ret != Z_OK)
    {
      const char *reason = zxp_error_char_zp(ret);
      return zxp_raise(env, __FILE__, __LINE__, reason);
    }

    term = zxp_binary_from_bytes(
        env, (const uint8_t *)z_string_data(z_loan(string)), z_string_len(z_loan(string)));
  }
  z_drop(z_move(string));
  return term;
}

ERL_NIF_TERM zxp_reply_err_from_zp_reply_err(ErlNifEnv *env, const z_loaned_reply_err_t *reply_err)
{
  ERL_NIF_TERM keys[] = {
      struct_atom,
      payload_atom,
      encoding_atom,
  };

  ERL_NIF_TERM values[] = {
      reply_error_module,
      zxp_binary_from_zp_bytes(env, z_reply_err_payload(reply_err)),
      zxp_binary_from_zp_encoding(env, z_reply_err_encoding(reply_err)),
  };

  for (size_t index = 0; index < 3; index++)
  {
    if (enif_is_exception(env, values[index]))
    {
      return values[index];
    }
  }

  ERL_NIF_TERM term;
  enif_make_map_from_arrays(env, keys, values, 3, &term);
  return term;
}
