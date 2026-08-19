#include <erl_nif.h>
#include <zenoh-pico.h>

#include "helper/helper.h"
#include "term.h"

static ERL_NIF_TERM zxp_priority(z_priority_t priority)
{
  switch (priority)
  {
  case Z_PRIORITY_REAL_TIME:
    return real_time_atom;
  case Z_PRIORITY_INTERACTIVE_HIGH:
    return interactive_high_atom;
  case Z_PRIORITY_INTERACTIVE_LOW:
    return interactive_low_atom;
  case Z_PRIORITY_DATA_HIGH:
    return data_high_atom;
  case Z_PRIORITY_DATA:
    return data_atom;
  case Z_PRIORITY_DATA_LOW:
    return data_low_atom;
  case Z_PRIORITY_BACKGROUND:
    return background_atom;
  default:
    return data_atom;
  }
}

static ERL_NIF_TERM zxp_binary_from_zp_bytes(ErlNifEnv *env, const z_loaned_bytes_t *bytes)
{
  if (bytes == NULL)
  {
    return nil_atom;
  }

  z_owned_slice_t slice;
  ERL_NIF_TERM term;
  {
    z_result_t ret = z_bytes_to_slice(bytes, &slice);
    if (ret != Z_OK)
    {
      const char *reason = zxp_error_char_zp(ret);
      return zxp_raise(env, __FILE__, __LINE__, reason);
    }
    term = zxp_binary_from_bytes(env, z_slice_data(z_loan(slice)), z_slice_len(z_loan(slice)));
  }
  z_drop(z_move(slice));
  return term;
}

static ERL_NIF_TERM zxp_binary_from_zp_keyexpr(ErlNifEnv *env, const z_loaned_keyexpr_t *keyexpr)
{
  z_view_string_t string;
  ERL_NIF_TERM term;
  {
    z_result_t ret = z_keyexpr_as_view_string(keyexpr, &string);
    if (ret != Z_OK)
    {
      const char *reason = zxp_error_char_zp(ret);
      return zxp_raise(env, __FILE__, __LINE__, reason);
    }

    term = zxp_binary_from_bytes(
        env, (const uint8_t *)z_string_data(z_loan(string)), z_string_len(z_loan(string)));
  }
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

ERL_NIF_TERM zxp_sample_from_zp_sample(ErlNifEnv *env, const z_loaned_sample_t *sample)
{
  ERL_NIF_TERM keys[] = {
      struct_atom,
      attachment_atom,
      congestion_control_atom,
      encoding_atom,
      express_atom,
      key_expr_atom,
      kind_atom,
      payload_atom,
      priority_atom,
      timestamp_atom,
  };

  ERL_NIF_TERM values[] = {
      sample_module,
      zxp_binary_from_zp_bytes(env, z_sample_attachment(sample)),
      z_sample_congestion_control(sample) == Z_CONGESTION_CONTROL_BLOCK ? block_atom : drop_atom,
      zxp_binary_from_zp_encoding(env, z_sample_encoding(sample)),
      z_sample_express(sample) ? true_atom : false_atom,
      zxp_binary_from_zp_keyexpr(env, z_sample_keyexpr(sample)),
      z_sample_kind(sample) == Z_SAMPLE_KIND_DELETE ? delete_atom : put_atom,
      zxp_binary_from_zp_bytes(env, z_sample_payload(sample)),
      zxp_priority(z_sample_priority(sample)),
      nil_atom,
  };

  for (size_t index = 0; index < 10; index++)
  {
    if (enif_is_exception(env, values[index]))
    {
      return values[index];
    }
  }

  ERL_NIF_TERM term;
  enif_make_map_from_arrays(env, keys, values, 10, &term);
  return term;
}
