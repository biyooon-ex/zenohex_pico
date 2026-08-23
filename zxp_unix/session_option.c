#include <erl_nif.h>
#include <zenoh-pico.h>

#include "session_option.h"
#include "term.h"
#include "timestamp.h"

static bool zxp_session_put_options_init(ErlNifEnv *env, ERL_NIF_TERM term,
                                         zxp_session_put_options_t *put_options)
{
  z_put_options_t *options = &put_options->options;
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
      if (z_encoding_from_substr(&put_options->encoding,
                                 (const char *)encoding_binary.data,
                                 encoding_binary.size) != Z_OK)
      {
        return false;
      }
      options->encoding = z_move(put_options->encoding);
    }
    else if (enif_is_identical(tuple[0], attachment_atom))
    {
      ErlNifBinary attachment_binary;
      if (!enif_inspect_binary(env, tuple[1], &attachment_binary))
      {
        return false;
      }
      z_bytes_drop(options->attachment);
      if (z_bytes_copy_from_buf(
              &put_options->attachment, attachment_binary.data, attachment_binary.size) != Z_OK)
      {
        return false;
      }
      options->attachment = z_move(put_options->attachment);
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
    else if (enif_is_identical(tuple[0], timestamp_atom))
    {
      ErlNifBinary timestamp_binary;
      if (!enif_inspect_binary(env, tuple[1], &timestamp_binary) ||
          !zxp_timestamp_from_binary(&timestamp_binary, &put_options->timestamp))
      {
        return false;
      }
      options->timestamp = &put_options->timestamp;
    }
    else
    {
      return false;
    }
    term = tail;
  }

  return enif_is_empty_list(env, term);
}

zxp_session_put_options_t *zxp_session_put_options_new(ErlNifEnv *env, ERL_NIF_TERM term)
{
  zxp_session_put_options_t *put_options = enif_alloc(sizeof(*put_options));
  if (put_options == NULL)
  {
    return NULL;
  }

  z_put_options_default(&put_options->options);
  put_options->timestamp = _z_timestamp_null();
  if (!zxp_session_put_options_init(env, term, put_options))
  {
    zxp_session_put_options_drop(put_options);
    return NULL;
  }

  return put_options;
}

void zxp_session_put_options_drop(zxp_session_put_options_t *put_options)
{
  z_encoding_drop(put_options->options.encoding);
  z_bytes_drop(put_options->options.attachment);
  enif_free(put_options);
}

z_put_options_t *zxp_session_put_options_loan(zxp_session_put_options_t *put_options)
{
  return &put_options->options;
}
