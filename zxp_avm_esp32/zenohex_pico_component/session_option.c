#include <atom.h>
#include <defaultatoms.h>
#include <term.h>
#include <zenoh-pico.h>

#include "session_option.h"
#include "timestamp.h"

static term encoding_atom;
static term payload_atom;
static term attachment_atom;
static term consolidation_atom;
static term congestion_control_atom;
static term priority_atom;
static term express_atom;
static term timestamp_atom;
static term target_atom;
static term accept_replies_atom;
static term query_timeout_atom;
static term auto_atom;
static term none_atom;
static term monotonic_atom;
static term latest_atom;
static term block_atom;
static term drop_atom;
static term best_matching_atom;
static term all_atom;
static term all_complete_atom;
static term matching_query_atom;
static term any_atom;
static term real_time_atom;
static term interactive_high_atom;
static term interactive_low_atom;
static term data_high_atom;
static term data_atom;
static term data_low_atom;
static term background_atom;

#define ZXP_ATOM(global, size, name) globalcontext_make_atom(global, ATOM_STR(size, name))

void zxp_session_option_init_atoms(GlobalContext *global)
{
  encoding_atom = ZXP_ATOM(global, "\x8", "encoding");
  payload_atom = ZXP_ATOM(global, "\x7", "payload");
  attachment_atom = ZXP_ATOM(global, "\xA", "attachment");
  consolidation_atom = ZXP_ATOM(global, "\xD", "consolidation");
  congestion_control_atom = ZXP_ATOM(global, "\x12", "congestion_control");
  priority_atom = ZXP_ATOM(global, "\x8", "priority");
  express_atom = ZXP_ATOM(global, "\x7", "express");
  timestamp_atom = ZXP_ATOM(global, "\x9", "timestamp");
  target_atom = ZXP_ATOM(global, "\x6", "target");
  accept_replies_atom = ZXP_ATOM(global, "\xE", "accept_replies");
  query_timeout_atom = ZXP_ATOM(global, "\xD", "query_timeout");
  auto_atom = ZXP_ATOM(global, "\x4", "auto");
  none_atom = ZXP_ATOM(global, "\x4", "none");
  monotonic_atom = ZXP_ATOM(global, "\x9", "monotonic");
  latest_atom = ZXP_ATOM(global, "\x6", "latest");
  block_atom = ZXP_ATOM(global, "\x5", "block");
  drop_atom = ZXP_ATOM(global, "\x4", "drop");
  best_matching_atom = ZXP_ATOM(global, "\xD", "best_matching");
  all_atom = ZXP_ATOM(global, "\x3", "all");
  all_complete_atom = ZXP_ATOM(global, "\xC", "all_complete");
  matching_query_atom = ZXP_ATOM(global, "\xE", "matching_query");
  any_atom = ZXP_ATOM(global, "\x3", "any");
  real_time_atom = ZXP_ATOM(global, "\x9", "real_time");
  interactive_high_atom = ZXP_ATOM(global, "\x10", "interactive_high");
  interactive_low_atom = ZXP_ATOM(global, "\xF", "interactive_low");
  data_high_atom = ZXP_ATOM(global, "\x9", "data_high");
  data_atom = ZXP_ATOM(global, "\x4", "data");
  data_low_atom = ZXP_ATOM(global, "\x8", "data_low");
  background_atom = ZXP_ATOM(global, "\xA", "background");
}

static bool zxp_set_priority(term value, z_priority_t *priority)
{
  if (value == real_time_atom) *priority = Z_PRIORITY_REAL_TIME;
  else if (value == interactive_high_atom) *priority = Z_PRIORITY_INTERACTIVE_HIGH;
  else if (value == interactive_low_atom) *priority = Z_PRIORITY_INTERACTIVE_LOW;
  else if (value == data_high_atom) *priority = Z_PRIORITY_DATA_HIGH;
  else if (value == data_atom) *priority = Z_PRIORITY_DATA;
  else if (value == data_low_atom) *priority = Z_PRIORITY_DATA_LOW;
  else if (value == background_atom) *priority = Z_PRIORITY_BACKGROUND;
  else return false;
  return true;
}

bool zxp_session_put_options_init(term option_list, zxp_session_put_options_t *put_options)
{
  z_put_options_default(&put_options->options);
  z_internal_null(&put_options->encoding);
  z_internal_null(&put_options->attachment);
  put_options->timestamp = _z_timestamp_null();

  while (term_is_nonempty_list(option_list)) {
    term option = term_get_list_head(option_list);
    option_list = term_get_list_tail(option_list);
    if (!term_is_tuple(option) || term_get_tuple_arity(option) != 2) return false;

    term key = term_get_tuple_element(option, 0);
    term value = term_get_tuple_element(option, 1);
    z_result_t result;
    if (key == encoding_atom && term_is_binary(value)) {
      z_encoding_drop(put_options->options.encoding);
      result = z_encoding_from_substr(
          &put_options->encoding, term_binary_data(value), term_binary_size(value));
      if (result != Z_OK) return false;
      put_options->options.encoding = z_move(put_options->encoding);
    } else if (key == attachment_atom && term_is_binary(value)) {
      z_bytes_drop(put_options->options.attachment);
      result = z_bytes_copy_from_buf(&put_options->attachment,
          (const uint8_t *) term_binary_data(value), term_binary_size(value));
      if (result != Z_OK) return false;
      put_options->options.attachment = z_move(put_options->attachment);
    } else if (key == congestion_control_atom && value == block_atom) {
      put_options->options.congestion_control = Z_CONGESTION_CONTROL_BLOCK;
    } else if (key == congestion_control_atom && value == drop_atom) {
      put_options->options.congestion_control = Z_CONGESTION_CONTROL_DROP;
    } else if (key == priority_atom) {
      if (!zxp_set_priority(value, &put_options->options.priority)) return false;
    } else if (key == express_atom && value == TRUE_ATOM) {
      put_options->options.is_express = true;
    } else if (key == express_atom && value == FALSE_ATOM) {
      put_options->options.is_express = false;
    } else if (key == timestamp_atom && term_is_binary(value)) {
      if (!zxp_timestamp_from_binary(term_binary_data(value), term_binary_size(value),
              &put_options->timestamp)) return false;
      put_options->options.timestamp = &put_options->timestamp;
    } else {
      return false;
    }
  }
  return term_is_nil(option_list);
}

void zxp_session_put_options_drop(zxp_session_put_options_t *put_options)
{
  z_encoding_drop(put_options->options.encoding);
  z_bytes_drop(put_options->options.attachment);
}

bool zxp_session_get_options_init(term option_list, zxp_session_get_options_t *get_options)
{
  z_get_options_default(&get_options->options);
  z_internal_null(&get_options->payload);
  z_internal_null(&get_options->encoding);
  z_internal_null(&get_options->attachment);

  while (term_is_nonempty_list(option_list)) {
    term option = term_get_list_head(option_list);
    option_list = term_get_list_tail(option_list);
    if (!term_is_tuple(option) || term_get_tuple_arity(option) != 2) return false;

    term key = term_get_tuple_element(option, 0);
    term value = term_get_tuple_element(option, 1);
    z_result_t result;
    if (key == payload_atom && term_is_binary(value)) {
      z_bytes_drop(get_options->options.payload);
      result = z_bytes_copy_from_buf(&get_options->payload,
          (const uint8_t *) term_binary_data(value), term_binary_size(value));
      if (result != Z_OK) return false;
      get_options->options.payload = z_move(get_options->payload);
    } else if (key == encoding_atom && term_is_binary(value)) {
      z_encoding_drop(get_options->options.encoding);
      result = z_encoding_from_substr(&get_options->encoding,
          term_binary_data(value), term_binary_size(value));
      if (result != Z_OK) return false;
      get_options->options.encoding = z_move(get_options->encoding);
    } else if (key == attachment_atom && term_is_binary(value)) {
      z_bytes_drop(get_options->options.attachment);
      result = z_bytes_copy_from_buf(&get_options->attachment,
          (const uint8_t *) term_binary_data(value), term_binary_size(value));
      if (result != Z_OK) return false;
      get_options->options.attachment = z_move(get_options->attachment);
    } else if (key == consolidation_atom && value == auto_atom) {
      get_options->options.consolidation.mode = Z_CONSOLIDATION_MODE_AUTO;
    } else if (key == consolidation_atom && value == none_atom) {
      get_options->options.consolidation.mode = Z_CONSOLIDATION_MODE_NONE;
    } else if (key == consolidation_atom && value == monotonic_atom) {
      get_options->options.consolidation.mode = Z_CONSOLIDATION_MODE_MONOTONIC;
    } else if (key == consolidation_atom && value == latest_atom) {
      get_options->options.consolidation.mode = Z_CONSOLIDATION_MODE_LATEST;
    } else if (key == congestion_control_atom && value == block_atom) {
      get_options->options.congestion_control = Z_CONGESTION_CONTROL_BLOCK;
    } else if (key == congestion_control_atom && value == drop_atom) {
      get_options->options.congestion_control = Z_CONGESTION_CONTROL_DROP;
    } else if (key == priority_atom) {
      if (!zxp_set_priority(value, &get_options->options.priority)) return false;
    } else if (key == express_atom && value == TRUE_ATOM) {
      get_options->options.is_express = true;
    } else if (key == express_atom && value == FALSE_ATOM) {
      get_options->options.is_express = false;
    } else if (key == target_atom && value == best_matching_atom) {
      get_options->options.target = Z_QUERY_TARGET_BEST_MATCHING;
    } else if (key == target_atom && value == all_atom) {
      get_options->options.target = Z_QUERY_TARGET_ALL;
    } else if (key == target_atom && value == all_complete_atom) {
      get_options->options.target = Z_QUERY_TARGET_ALL_COMPLETE;
    } else if (key == accept_replies_atom && value == matching_query_atom) {
      get_options->options.accept_replies = Z_REPLY_KEYEXPR_MATCHING_QUERY;
    } else if (key == accept_replies_atom && value == any_atom) {
      get_options->options.accept_replies = Z_REPLY_KEYEXPR_ANY;
    } else if (key == query_timeout_atom && term_is_uint64(value)) {
      get_options->options.timeout_ms = term_to_uint64(value);
    } else {
      return false;
    }
  }
  return term_is_nil(option_list);
}

void zxp_session_get_options_drop(zxp_session_get_options_t *get_options)
{
  z_bytes_drop(get_options->options.payload);
  z_encoding_drop(get_options->options.encoding);
  z_bytes_drop(get_options->options.attachment);
}
