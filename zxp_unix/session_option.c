#include <erl_nif.h>
#include <zenoh-pico.h>

#include "session_option.h"
#include "term.h"
#include "timestamp.h"

////
// session_put_option
//

bool zxp_session_put_options_new(ErlNifEnv *env, zxp_session_put_options_t **put_options,
                                 ERL_NIF_TERM *error)
{
  *put_options = enif_alloc(sizeof(**put_options));
  if (*put_options == NULL)
  {
    *error = zxp_raise_null_pointer(env, __FILE__, __LINE__);
    return false;
  }

  z_internal_null(&(*put_options)->encoding);
  z_internal_null(&(*put_options)->attachment);
  z_put_options_default(&(*put_options)->options);
  (*put_options)->timestamp = _z_timestamp_null();
  return true;
}

bool zxp_session_put_options_init(ErlNifEnv *env, ERL_NIF_TERM term,
                                  zxp_session_put_options_t *put_options, ERL_NIF_TERM *error)
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
      *error = enif_make_badarg(env);
      return false;
    }

    z_result_t ret;
    if (enif_is_identical(tuple[0], encoding_atom))
    {
      ErlNifBinary encoding_binary;
      if (!enif_inspect_binary(env, tuple[1], &encoding_binary))
      {
        *error = enif_make_badarg(env);
        return false;
      }
      z_encoding_drop(options->encoding);
      ret = z_encoding_from_substr(
          &put_options->encoding, (const char *)encoding_binary.data, encoding_binary.size);
      if (ret != Z_OK)
      {
        *error = zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
        return false;
      }
      options->encoding = z_move(put_options->encoding);
    }
    else if (enif_is_identical(tuple[0], attachment_atom))
    {
      ErlNifBinary attachment_binary;
      if (!enif_inspect_binary(env, tuple[1], &attachment_binary))
      {
        *error = enif_make_badarg(env);
        return false;
      }
      z_bytes_drop(options->attachment);
      ret = z_bytes_copy_from_buf(
          &put_options->attachment, attachment_binary.data, attachment_binary.size);
      if (ret != Z_OK)
      {
        *error = zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
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
        *error = enif_make_badarg(env);
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
        *error = enif_make_badarg(env);
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
        *error = enif_make_badarg(env);
        return false;
      }
    }
    else if (enif_is_identical(tuple[0], timestamp_atom))
    {
      ErlNifBinary timestamp_binary;
      if (!enif_inspect_binary(env, tuple[1], &timestamp_binary) ||
          !zxp_timestamp_from_binary(&timestamp_binary, &put_options->timestamp))
      {
        *error = enif_make_badarg(env);
        return false;
      }
      options->timestamp = &put_options->timestamp;
    }
    else
    {
      *error = enif_make_badarg(env);
      return false;
    }
    term = tail;
  }

  if (!enif_is_empty_list(env, term))
  {
    *error = enif_make_badarg(env);
    return false;
  }

  return true;
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

////
// session_get_option
//

bool zxp_session_get_options_new(ErlNifEnv *env, zxp_session_get_options_t **get_options,
                                 ERL_NIF_TERM *error)
{
  *get_options = enif_alloc(sizeof(**get_options));
  if (*get_options == NULL)
  {
    *error = zxp_raise_null_pointer(env, __FILE__, __LINE__);
    return false;
  }

  z_internal_null(&(*get_options)->payload);
  z_internal_null(&(*get_options)->encoding);
  z_internal_null(&(*get_options)->attachment);
  z_get_options_default(&(*get_options)->options);
  return true;
}

bool zxp_session_get_options_init(ErlNifEnv *env, ERL_NIF_TERM term,
                                  zxp_session_get_options_t *get_options, ERL_NIF_TERM *error)
{
  // For embedded deployments, we determined that queries from the same Zenoh-Pico
  // session on a device do not need to reach queryables on that device. We do not
  // use a Zenoh-Pico library with Z_FEATURE_LOCAL_QUERYABLE enabled, so
  // allowed_destination is not supported.
  z_get_options_t *options = &get_options->options;
  ERL_NIF_TERM head;
  ERL_NIF_TERM tail;

  while (enif_get_list_cell(env, term, &head, &tail))
  {
    const ERL_NIF_TERM *tuple;
    int arity;
    if (!enif_get_tuple(env, head, &arity, &tuple) || arity != 2)
    {
      *error = enif_make_badarg(env);
      return false;
    }

    z_result_t ret;
    if (enif_is_identical(tuple[0], payload_atom))
    {
      ErlNifBinary payload_binary;
      if (!enif_inspect_binary(env, tuple[1], &payload_binary))
      {
        *error = enif_make_badarg(env);
        return false;
      }
      z_bytes_drop(options->payload);
      ret = z_bytes_copy_from_buf(&get_options->payload, payload_binary.data, payload_binary.size);
      if (ret != Z_OK)
      {
        *error = zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
        return false;
      }
      options->payload = z_move(get_options->payload);
    }
    else if (enif_is_identical(tuple[0], encoding_atom))
    {
      ErlNifBinary encoding_binary;
      if (!enif_inspect_binary(env, tuple[1], &encoding_binary))
      {
        *error = enif_make_badarg(env);
        return false;
      }
      z_encoding_drop(options->encoding);
      ret = z_encoding_from_substr(
          &get_options->encoding, (const char *)encoding_binary.data, encoding_binary.size);
      if (ret != Z_OK)
      {
        *error = zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
        return false;
      }
      options->encoding = z_move(get_options->encoding);
    }
    else if (enif_is_identical(tuple[0], attachment_atom))
    {
      ErlNifBinary attachment_binary;
      if (!enif_inspect_binary(env, tuple[1], &attachment_binary))
      {
        *error = enif_make_badarg(env);
        return false;
      }
      z_bytes_drop(options->attachment);
      ret = z_bytes_copy_from_buf(
          &get_options->attachment, attachment_binary.data, attachment_binary.size);
      if (ret != Z_OK)
      {
        *error = zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
        return false;
      }
      options->attachment = z_move(get_options->attachment);
    }
    else if (enif_is_identical(tuple[0], consolidation_atom))
    {
      if (enif_is_identical(tuple[1], auto_atom))
      {
        options->consolidation.mode = Z_CONSOLIDATION_MODE_AUTO;
      }
      else if (enif_is_identical(tuple[1], none_atom))
      {
        options->consolidation.mode = Z_CONSOLIDATION_MODE_NONE;
      }
      else if (enif_is_identical(tuple[1], monotonic_atom))
      {
        options->consolidation.mode = Z_CONSOLIDATION_MODE_MONOTONIC;
      }
      else if (enif_is_identical(tuple[1], latest_atom))
      {
        options->consolidation.mode = Z_CONSOLIDATION_MODE_LATEST;
      }
      else
      {
        *error = enif_make_badarg(env);
        return false;
      }
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
        *error = enif_make_badarg(env);
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
        *error = enif_make_badarg(env);
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
        *error = enif_make_badarg(env);
        return false;
      }
    }
    else if (enif_is_identical(tuple[0], target_atom))
    {
      if (enif_is_identical(tuple[1], best_matching_atom))
      {
        options->target = Z_QUERY_TARGET_BEST_MATCHING;
      }
      else if (enif_is_identical(tuple[1], all_atom))
      {
        options->target = Z_QUERY_TARGET_ALL;
      }
      else if (enif_is_identical(tuple[1], all_complete_atom))
      {
        options->target = Z_QUERY_TARGET_ALL_COMPLETE;
      }
      else
      {
        *error = enif_make_badarg(env);
        return false;
      }
    }
    else if (enif_is_identical(tuple[0], accept_replies_atom))
    {
      if (enif_is_identical(tuple[1], matching_query_atom))
      {
        options->accept_replies = Z_REPLY_KEYEXPR_MATCHING_QUERY;
      }
      else if (enif_is_identical(tuple[1], any_atom))
      {
        options->accept_replies = Z_REPLY_KEYEXPR_ANY;
      }
      else
      {
        *error = enif_make_badarg(env);
        return false;
      }
    }
    else if (enif_is_identical(tuple[0], query_timeout_atom))
    {
      if (!enif_get_uint64(env, tuple[1], &options->timeout_ms))
      {
        *error = enif_make_badarg(env);
        return false;
      }
    }
    else
    {
      *error = enif_make_badarg(env);
      return false;
    }
    term = tail;
  }

  if (!enif_is_empty_list(env, term))
  {
    *error = enif_make_badarg(env);
    return false;
  }

  return true;
}

void zxp_session_get_options_drop(zxp_session_get_options_t *get_options)
{
  z_bytes_drop(get_options->options.payload);
  z_encoding_drop(get_options->options.encoding);
  z_bytes_drop(get_options->options.attachment);
  enif_free(get_options);
}

z_get_options_t *zxp_session_get_options_loan(zxp_session_get_options_t *get_options)
{
  return &get_options->options;
}
