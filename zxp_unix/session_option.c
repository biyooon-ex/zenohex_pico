#include <erl_nif.h>
#include <zenoh-pico.h>

#include "term.h"

void zxp_session_put_options_drop(z_put_options_t *options)
{
  z_encoding_drop(options->encoding);
  z_bytes_drop(options->attachment);
}

bool zxp_session_put_options(ErlNifEnv *env, ERL_NIF_TERM term, z_put_options_t *options,
                             z_owned_encoding_t *encoding, z_owned_bytes_t *attachment)
{
  ERL_NIF_TERM head;
  ERL_NIF_TERM tail;

  while (enif_get_list_cell(env, term, &head, &tail))
  {
    const ERL_NIF_TERM *tuple;
    int arity;
    if (!enif_get_tuple(env, head, &arity, &tuple) || arity != 2)
    {
      return false;
    }

    if (enif_is_identical(tuple[0], encoding_atom))
    {
      ErlNifBinary encoding_binary;
      if (!enif_inspect_binary(env, tuple[1], &encoding_binary))
      {
        return false;
      }
      z_encoding_drop(options->encoding);
      if (z_encoding_from_substr(
              encoding, (const char *)encoding_binary.data, encoding_binary.size) != Z_OK)
      {
        return false;
      }
      options->encoding = z_move(*encoding);
    }
    else if (enif_is_identical(tuple[0], attachment_atom))
    {
      ErlNifBinary attachment_binary;
      if (!enif_inspect_binary(env, tuple[1], &attachment_binary))
      {
        return false;
      }
      z_bytes_drop(options->attachment);
      if (z_bytes_copy_from_buf(attachment, attachment_binary.data, attachment_binary.size) != Z_OK)
      {
        return false;
      }
      options->attachment = z_move(*attachment);
    }
    else if (enif_is_identical(tuple[0], congestion_control_atom))
    {
      if (enif_is_identical(tuple[1], block_atom))
      {
        options->congestion_control = Z_CONGESTION_CONTROL_BLOCK;
      }
      else if (enif_is_identical(tuple[1], drop_atom))
      {
        options->congestion_control = Z_CONGESTION_CONTROL_DROP;
      }
      else
      {
        return false;
      }
    }
    else if (enif_is_identical(tuple[0], priority_atom))
    {
      if (enif_is_identical(tuple[1], real_time_atom))
      {
        options->priority = Z_PRIORITY_REAL_TIME;
      }
      else if (enif_is_identical(tuple[1], interactive_high_atom))
      {
        options->priority = Z_PRIORITY_INTERACTIVE_HIGH;
      }
      else if (enif_is_identical(tuple[1], interactive_low_atom))
      {
        options->priority = Z_PRIORITY_INTERACTIVE_LOW;
      }
      else if (enif_is_identical(tuple[1], data_high_atom))
      {
        options->priority = Z_PRIORITY_DATA_HIGH;
      }
      else if (enif_is_identical(tuple[1], data_atom))
      {
        options->priority = Z_PRIORITY_DATA;
      }
      else if (enif_is_identical(tuple[1], data_low_atom))
      {
        options->priority = Z_PRIORITY_DATA_LOW;
      }
      else if (enif_is_identical(tuple[1], background_atom))
      {
        options->priority = Z_PRIORITY_BACKGROUND;
      }
      else
      {
        return false;
      }
    }
    else if (enif_is_identical(tuple[0], express_atom))
    {
      if (enif_is_identical(tuple[1], true_atom))
      {
        options->is_express = true;
      }
      else if (enif_is_identical(tuple[1], false_atom))
      {
        options->is_express = false;
      }
      else
      {
        return false;
      }
    }
    else
    {
      return false;
    }
    term = tail;
  }

  return enif_is_empty_list(env, term);
}
