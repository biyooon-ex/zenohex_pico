#include <defaultatoms.h>
#include <stdlib.h>
#include <term.h>
#include <zenoh-pico.h>

#include "avm_compat.h"
#include "session_option.h"
#include "timestamp.h"

////
// session_put_option
//

bool zxp_session_put_options_new(Context *ctx, zxp_session_put_options_t **put_options, term *error)
{
  *put_options = malloc(sizeof(**put_options));
  if (*put_options == NULL)
  {
    *error = zxp_raise(ctx, "malloc/1 failed");
    return false;
  }
  z_internal_null(&(*put_options)->encoding);
  z_internal_null(&(*put_options)->attachment);
  z_put_options_default(&(*put_options)->options);
  (*put_options)->timestamp = _z_timestamp_null();
  return true;
}

bool zxp_session_put_options_init(Context *ctx, term option_list,
                                  zxp_session_put_options_t *put_options, term *error)
{
  z_put_options_t *options = &put_options->options;
  while (term_is_nonempty_list(option_list))
  {
    term option = term_get_list_head(option_list);
    option_list = term_get_list_tail(option_list);
    if (!term_is_tuple(option) || term_get_tuple_arity(option) != 2)
    {
      *error = BADARG_ATOM;
      return false;
    }

    term key = term_get_tuple_element(option, 0);
    term value = term_get_tuple_element(option, 1);
    z_result_t ret;
    if (key == encoding_atom)
    {
      if (!term_is_binary(value))
      {
        *error = BADARG_ATOM;
        return false;
      }
      z_encoding_drop(options->encoding);
      ret = z_encoding_from_substr(
          &put_options->encoding, term_binary_data(value), term_binary_size(value));
      if (ret != Z_OK)
      {
        *error = zxp_error_tuple_zp(ctx, ret);
        return false;
      }
      options->encoding = z_move(put_options->encoding);
    }
    else if (key == attachment_atom)
    {
      if (!term_is_binary(value))
      {
        *error = BADARG_ATOM;
        return false;
      }
      z_bytes_drop(options->attachment);
      ret = z_bytes_copy_from_buf(&put_options->attachment,
                                  (const uint8_t *)term_binary_data(value),
                                  term_binary_size(value));
      if (ret != Z_OK)
      {
        *error = zxp_error_tuple_zp(ctx, ret);
        return false;
      }
      options->attachment = z_move(put_options->attachment);
    }
    else if (key == congestion_control_atom)
    {
      if (value == block_atom)
      {
        options->congestion_control = Z_CONGESTION_CONTROL_BLOCK;
      }
      else if (value == drop_atom)
      {
        options->congestion_control = Z_CONGESTION_CONTROL_DROP;
      }
      else
      {
        *error = BADARG_ATOM;
        return false;
      }
    }
    else if (key == priority_atom)
    {
      if (value == real_time_atom)
      {
        options->priority = Z_PRIORITY_REAL_TIME;
      }
      else if (value == interactive_high_atom)
      {
        options->priority = Z_PRIORITY_INTERACTIVE_HIGH;
      }
      else if (value == interactive_low_atom)
      {
        options->priority = Z_PRIORITY_INTERACTIVE_LOW;
      }
      else if (value == data_high_atom)
      {
        options->priority = Z_PRIORITY_DATA_HIGH;
      }
      else if (value == data_atom)
      {
        options->priority = Z_PRIORITY_DATA;
      }
      else if (value == data_low_atom)
      {
        options->priority = Z_PRIORITY_DATA_LOW;
      }
      else if (value == background_atom)
      {
        options->priority = Z_PRIORITY_BACKGROUND;
      }
      else
      {
        *error = BADARG_ATOM;
        return false;
      }
    }
    else if (key == express_atom)
    {
      if (value == TRUE_ATOM)
      {
        options->is_express = true;
      }
      else if (value == FALSE_ATOM)
      {
        options->is_express = false;
      }
      else
      {
        *error = BADARG_ATOM;
        return false;
      }
    }
    else if (key == timestamp_atom)
    {
      if (!term_is_binary(value))
      {
        *error = BADARG_ATOM;
        return false;
      }
      if (!zxp_timestamp_from_binary(
              term_binary_data(value), term_binary_size(value), &put_options->timestamp))
      {
        *error = BADARG_ATOM;
        return false;
      }
      options->timestamp = &put_options->timestamp;
    }
    else
    {
      *error = BADARG_ATOM;
      return false;
    }
  }
  if (!term_is_nil(option_list))
  {
    *error = BADARG_ATOM;
    return false;
  }
  return true;
}

void zxp_session_put_options_drop(zxp_session_put_options_t *put_options)
{
  z_encoding_drop(put_options->options.encoding);
  z_bytes_drop(put_options->options.attachment);
  free(put_options);
}

z_put_options_t *zxp_session_put_options_loan(zxp_session_put_options_t *put_options)
{
  return &put_options->options;
}

////
// session_get_option
//

bool zxp_session_get_options_new(Context *ctx, zxp_session_get_options_t **get_options, term *error)
{
  *get_options = malloc(sizeof(**get_options));
  if (*get_options == NULL)
  {
    *error = zxp_raise(ctx, "malloc/1 failed");
    return false;
  }
  z_internal_null(&(*get_options)->payload);
  z_internal_null(&(*get_options)->encoding);
  z_internal_null(&(*get_options)->attachment);
  z_get_options_default(&(*get_options)->options);
  return true;
}

bool zxp_session_get_options_init(Context *ctx, term option_list,
                                  zxp_session_get_options_t *get_options, term *error)
{
  z_get_options_t *options = &get_options->options;
  while (term_is_nonempty_list(option_list))
  {
    term option = term_get_list_head(option_list);
    option_list = term_get_list_tail(option_list);
    if (!term_is_tuple(option) || term_get_tuple_arity(option) != 2)
    {
      *error = BADARG_ATOM;
      return false;
    }

    term key = term_get_tuple_element(option, 0);
    term value = term_get_tuple_element(option, 1);
    z_result_t ret;
    if (key == payload_atom)
    {
      if (!term_is_binary(value))
      {
        *error = BADARG_ATOM;
        return false;
      }
      z_bytes_drop(options->payload);
      ret = z_bytes_copy_from_buf(
          &get_options->payload, (const uint8_t *)term_binary_data(value), term_binary_size(value));
      if (ret != Z_OK)
      {
        *error = zxp_error_tuple_zp(ctx, ret);
        return false;
      }
      options->payload = z_move(get_options->payload);
    }
    else if (key == encoding_atom)
    {
      if (!term_is_binary(value))
      {
        *error = BADARG_ATOM;
        return false;
      }
      z_encoding_drop(options->encoding);
      ret = z_encoding_from_substr(
          &get_options->encoding, term_binary_data(value), term_binary_size(value));
      if (ret != Z_OK)
      {
        *error = zxp_error_tuple_zp(ctx, ret);
        return false;
      }
      options->encoding = z_move(get_options->encoding);
    }
    else if (key == attachment_atom)
    {
      if (!term_is_binary(value))
      {
        *error = BADARG_ATOM;
        return false;
      }
      z_bytes_drop(options->attachment);
      ret = z_bytes_copy_from_buf(&get_options->attachment,
                                  (const uint8_t *)term_binary_data(value),
                                  term_binary_size(value));
      if (ret != Z_OK)
      {
        *error = zxp_error_tuple_zp(ctx, ret);
        return false;
      }
      options->attachment = z_move(get_options->attachment);
    }
    else if (key == consolidation_atom)
    {
      if (value == auto_atom)
      {
        options->consolidation.mode = Z_CONSOLIDATION_MODE_AUTO;
      }
      else if (value == none_atom)
      {
        options->consolidation.mode = Z_CONSOLIDATION_MODE_NONE;
      }
      else if (value == monotonic_atom)
      {
        options->consolidation.mode = Z_CONSOLIDATION_MODE_MONOTONIC;
      }
      else if (value == latest_atom)
      {
        options->consolidation.mode = Z_CONSOLIDATION_MODE_LATEST;
      }
      else
      {
        *error = BADARG_ATOM;
        return false;
      }
    }
    else if (key == congestion_control_atom)
    {
      if (value == block_atom)
      {
        options->congestion_control = Z_CONGESTION_CONTROL_BLOCK;
      }
      else if (value == drop_atom)
      {
        options->congestion_control = Z_CONGESTION_CONTROL_DROP;
      }
      else
      {
        *error = BADARG_ATOM;
        return false;
      }
    }
    else if (key == priority_atom)
    {
      if (value == real_time_atom)
      {
        options->priority = Z_PRIORITY_REAL_TIME;
      }
      else if (value == interactive_high_atom)
      {
        options->priority = Z_PRIORITY_INTERACTIVE_HIGH;
      }
      else if (value == interactive_low_atom)
      {
        options->priority = Z_PRIORITY_INTERACTIVE_LOW;
      }
      else if (value == data_high_atom)
      {
        options->priority = Z_PRIORITY_DATA_HIGH;
      }
      else if (value == data_atom)
      {
        options->priority = Z_PRIORITY_DATA;
      }
      else if (value == data_low_atom)
      {
        options->priority = Z_PRIORITY_DATA_LOW;
      }
      else if (value == background_atom)
      {
        options->priority = Z_PRIORITY_BACKGROUND;
      }
      else
      {
        *error = BADARG_ATOM;
        return false;
      }
    }
    else if (key == express_atom)
    {
      if (value == TRUE_ATOM)
      {
        options->is_express = true;
      }
      else if (value == FALSE_ATOM)
      {
        options->is_express = false;
      }
      else
      {
        *error = BADARG_ATOM;
        return false;
      }
    }
    else if (key == target_atom)
    {
      if (value == best_matching_atom)
      {
        options->target = Z_QUERY_TARGET_BEST_MATCHING;
      }
      else if (value == all_atom)
      {
        options->target = Z_QUERY_TARGET_ALL;
      }
      else if (value == all_complete_atom)
      {
        options->target = Z_QUERY_TARGET_ALL_COMPLETE;
      }
      else
      {
        *error = BADARG_ATOM;
        return false;
      }
    }
    else if (key == accept_replies_atom)
    {
      if (value == matching_query_atom)
      {
        options->accept_replies = Z_REPLY_KEYEXPR_MATCHING_QUERY;
      }
      else if (value == any_atom)
      {
        options->accept_replies = Z_REPLY_KEYEXPR_ANY;
      }
      else
      {
        return false;
      }
    }
    else if (key == query_timeout_atom)
    {
      if (!term_is_uint64(value))
      {
        *error = BADARG_ATOM;
        return false;
      }
      options->timeout_ms = term_to_uint64(value);
    }
    else
    {
      *error = BADARG_ATOM;
      return false;
    }
  }
  if (!term_is_nil(option_list))
  {
    *error = BADARG_ATOM;
    return false;
  }
  return true;
}

void zxp_session_get_options_drop(zxp_session_get_options_t *get_options)
{
  z_bytes_drop(get_options->options.payload);
  z_encoding_drop(get_options->options.encoding);
  z_bytes_drop(get_options->options.attachment);
  free(get_options);
}

z_get_options_t *zxp_session_get_options_loan(zxp_session_get_options_t *get_options)
{
  return &get_options->options;
}
