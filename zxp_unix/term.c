#include <erl_nif.h>
#include <stdio.h>
#include <zenoh-pico.h>

#include "macro.h"

// primitive
ERL_NIF_TERM ok_atom;
ERL_NIF_TERM error_atom;
ERL_NIF_TERM not_found_atom;
ERL_NIF_TERM nil_atom;
ERL_NIF_TERM timeout_atom;
ERL_NIF_TERM struct_atom;
ERL_NIF_TERM true_atom;
ERL_NIF_TERM false_atom;

//
ERL_NIF_TERM payload_atom;
ERL_NIF_TERM encoding_atom;
ERL_NIF_TERM attachment_atom;
ERL_NIF_TERM express_atom;
ERL_NIF_TERM key_expr_atom;
ERL_NIF_TERM timestamp_atom;

// query option
ERL_NIF_TERM query_timeout_atom;
ERL_NIF_TERM accept_replies_atom;
ERL_NIF_TERM allowed_destination_atom;
ERL_NIF_TERM allowed_origin_atom;
ERL_NIF_TERM consolidation_atom;
ERL_NIF_TERM target_atom;

// kind
ERL_NIF_TERM kind_atom;
ERL_NIF_TERM delete_atom;
ERL_NIF_TERM put_atom;

// module
ERL_NIF_TERM sample_module;
ERL_NIF_TERM reply_error_module;

// congestion control
ERL_NIF_TERM congestion_control_atom;
ERL_NIF_TERM block_atom;
ERL_NIF_TERM drop_atom;

// consolidation
ERL_NIF_TERM auto_atom;
ERL_NIF_TERM none_atom;
ERL_NIF_TERM monotonic_atom;
ERL_NIF_TERM latest_atom;

// target
ERL_NIF_TERM best_matching_atom;
ERL_NIF_TERM all_atom;
ERL_NIF_TERM all_complete_atom;

// accept replies
ERL_NIF_TERM matching_query_atom;
ERL_NIF_TERM any_atom;

// allowed destination
ERL_NIF_TERM session_local_atom;
ERL_NIF_TERM remote_atom;

// priority
ERL_NIF_TERM priority_atom;
ERL_NIF_TERM real_time_atom;
ERL_NIF_TERM interactive_high_atom;
ERL_NIF_TERM interactive_low_atom;
ERL_NIF_TERM data_high_atom;
ERL_NIF_TERM data_atom;
ERL_NIF_TERM data_low_atom;
ERL_NIF_TERM background_atom;

static ERL_NIF_TERM zxp_error_binary(ErlNifEnv *env, const char *file, int line, const char *reason)
{
  int len = snprintf(NULL, 0, "%s at %s:%d", reason, file, line);
  if (len < 0)
  {
    return enif_make_tuple2(env, error_atom, enif_make_atom(env, "snprintf_failed"));
  }
  char *msg = enif_alloc(len + 1);
  if (msg == NULL)
  {
    return enif_make_tuple2(env, error_atom, enif_make_atom(env, "enif_alloc_failed"));
  }

  snprintf(msg, len + 1, "%s at %s:%d", reason, file, line);

  ErlNifBinary bin;
  if (!enif_alloc_binary(len, &bin))
  {
    enif_free(msg);
    return enif_make_tuple2(env, error_atom, enif_make_atom(env, "enif_alloc_binary_failed"));
  }

  memcpy(bin.data, msg, len);
  enif_free(msg);

  return enif_make_binary(env, &bin);
}

void zxp_init_atom(ErlNifEnv *env)
{
  // primitive
  ok_atom = enif_make_atom(env, "ok");
  error_atom = enif_make_atom(env, "error");
  not_found_atom = enif_make_atom(env, "not_found");
  nil_atom = enif_make_atom(env, "nil");
  timeout_atom = enif_make_atom(env, "timeout");
  struct_atom = enif_make_atom(env, "__struct__");
  true_atom = enif_make_atom(env, "true");
  false_atom = enif_make_atom(env, "false");

  //
  payload_atom = enif_make_atom(env, "payload");
  encoding_atom = enif_make_atom(env, "encoding");
  attachment_atom = enif_make_atom(env, "attachment");
  express_atom = enif_make_atom(env, "express");
  key_expr_atom = enif_make_atom(env, "key_expr");
  timestamp_atom = enif_make_atom(env, "timestamp");

  // query option
  query_timeout_atom = enif_make_atom(env, "query_timeout");
  accept_replies_atom = enif_make_atom(env, "accept_replies");
  allowed_destination_atom = enif_make_atom(env, "allowed_destination");
  allowed_origin_atom = enif_make_atom(env, "allowed_origin");
  consolidation_atom = enif_make_atom(env, "consolidation");
  target_atom = enif_make_atom(env, "target");

  // kind
  kind_atom = enif_make_atom(env, "kind");
  delete_atom = enif_make_atom(env, "delete");
  put_atom = enif_make_atom(env, "put");

  // module
  sample_module = enif_make_atom(env, "Elixir.ZenohexPico.Sample");
  reply_error_module = enif_make_atom(env, "Elixir.ZenohexPico.Query.ReplyError");

  // congestion control
  congestion_control_atom = enif_make_atom(env, "congestion_control");
  block_atom = enif_make_atom(env, "block");
  drop_atom = enif_make_atom(env, "drop");

  // consolidation
  auto_atom = enif_make_atom(env, "auto");
  none_atom = enif_make_atom(env, "none");
  monotonic_atom = enif_make_atom(env, "monotonic");
  latest_atom = enif_make_atom(env, "latest");

  // target
  best_matching_atom = enif_make_atom(env, "best_matching");
  all_atom = enif_make_atom(env, "all");
  all_complete_atom = enif_make_atom(env, "all_complete");

  // accept replies
  matching_query_atom = enif_make_atom(env, "matching_query");
  any_atom = enif_make_atom(env, "any");

  // allowed destination
  session_local_atom = enif_make_atom(env, "session_local");
  remote_atom = enif_make_atom(env, "remote");

  // priority
  priority_atom = enif_make_atom(env, "priority");
  real_time_atom = enif_make_atom(env, "real_time");
  interactive_high_atom = enif_make_atom(env, "interactive_high");
  interactive_low_atom = enif_make_atom(env, "interactive_low");
  data_high_atom = enif_make_atom(env, "data_high");
  data_atom = enif_make_atom(env, "data");
  data_low_atom = enif_make_atom(env, "data_low");
  background_atom = enif_make_atom(env, "background");
}

ERL_NIF_TERM zxp_raise(ErlNifEnv *env, const char *file, int line, const char *reason)
{
  ERL_NIF_TERM binary = zxp_error_binary(env, file, line, reason);
  return enif_raise_exception(env, binary);
}

ERL_NIF_TERM zxp_raise_null_pointer(ErlNifEnv *env, const char *file, int line)
{
  return zxp_raise(env, file, line, "null pointer");
}

const char *zxp_error_char_zp(z_result_t ret)
{
  const char *reason;

  switch (ret)
  {

  case Z_CHANNEL_DISCONNECTED:
    reason = "z_channel_disconnected";
    break;
  case Z_CHANNEL_NODATA:
    reason = "z_channel_nodata";
    break;
  case Z_NO_DATA_PROCESSED:
    reason = "z_no_data_processed";
    break;
  case _Z_RESOURCE_POSITIVE_REF_COUNT:
    reason = "_z_resource_positive_ref_count";
    break;
  case Z_SYNC_GROUP_CLOSED:
    reason = "z_sync_group_closed";
    break;
  case _Z_ERR_FAILED_TO_SPAWN_TASK:
    reason = "_z_err_failed_to_spawn_task";
    break;
  case _Z_ERR_TRANSPORT_RX_DURATION_EXPIRED:
    reason = "_z_err_transport_rx_duration_expired";
    break;
  case _Z_ERR_MESSAGE_DESERIALIZATION_FAILED:
    reason = "_z_err_message_deserialization_failed";
    break;
  case _Z_ERR_MESSAGE_SERIALIZATION_FAILED:
    reason = "_z_err_message_serialization_failed";
    break;
  case _Z_ERR_MESSAGE_UNEXPECTED:
    reason = "_z_err_message_unexpected";
    break;
  case _Z_ERR_MESSAGE_FLAG_UNEXPECTED:
    reason = "_z_err_message_flag_unexpected";
    break;
  case _Z_ERR_MESSAGE_ZENOH_DECLARATION_UNKNOWN:
    reason = "_z_err_message_zenoh_declaration_unknown";
    break;
  case _Z_ERR_MESSAGE_ZENOH_UNKNOWN:
    reason = "_z_err_message_zenoh_unknown";
    break;
  case _Z_ERR_MESSAGE_TRANSPORT_UNKNOWN:
    reason = "_z_err_message_transport_unknown";
    break;
  case _Z_ERR_MESSAGE_EXTENSION_MANDATORY_AND_UNKNOWN:
    reason = "_z_err_message_extension_mandatory_and_unknown";
    break;
  case _Z_ERR_ENTITY_DECLARATION_FAILED:
    reason = "_z_err_entity_declaration_failed";
    break;
  case _Z_ERR_ENTITY_UNKNOWN:
    reason = "_z_err_entity_unknown";
    break;
  case _Z_ERR_KEYEXPR_UNKNOWN:
    reason = "_z_err_keyexpr_unknown";
    break;
  case _Z_ERR_KEYEXPR_NOT_MATCH:
    reason = "_z_err_keyexpr_not_match";
    break;
  case _Z_ERR_QUERY_NOT_MATCH:
    reason = "_z_err_query_not_match";
    break;
  case _Z_ERR_TRANSPORT_NOT_AVAILABLE:
    reason = "_z_err_transport_not_available";
    break;
  case _Z_ERR_TRANSPORT_OPEN_FAILED:
    reason = "_z_err_transport_open_failed";
    break;
  case _Z_ERR_TRANSPORT_OPEN_SN_RESOLUTION:
    reason = "_z_err_transport_open_sn_resolution";
    break;
  case _Z_ERR_TRANSPORT_TX_FAILED:
    reason = "_z_err_transport_tx_failed";
    break;
  case _Z_ERR_TRANSPORT_RX_FAILED:
    reason = "_z_err_transport_rx_failed";
    break;
  case _Z_ERR_TRANSPORT_NO_SPACE:
    reason = "_z_err_transport_no_space";
    break;
  case _Z_ERR_TRANSPORT_NOT_ENOUGH_BYTES:
    reason = "_z_err_transport_not_enough_bytes";
    break;
  case _Z_ERR_CONFIG_FAILED_INSERT:
    reason = "_z_err_config_failed_insert";
    break;
  case _Z_ERR_CONFIG_UNSUPPORTED_CLIENT_MULTICAST:
    reason = "_z_err_config_unsupported_client_multicast";
    break;
  case _Z_ERR_CONFIG_UNSUPPORTED_PEER_UNICAST:
    reason = "_z_err_config_unsupported_peer_unicast";
    break;
  case _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN:
    reason = "_z_err_config_locator_schema_unknown";
    break;
  case _Z_ERR_CONFIG_LOCATOR_INVALID:
    reason = "_z_err_config_locator_invalid";
    break;
  case _Z_ERR_CONFIG_INVALID_MODE:
    reason = "_z_err_config_invalid_mode";
    break;
  case _Z_ERR_SCOUT_NO_RESULTS:
    reason = "_z_err_scout_no_results";
    break;
  case _Z_ERR_UNDERFLOW:
    reason = "_z_err_underflow";
    break;
  case _Z_ERR_SYSTEM_GENERIC:
    reason = "_z_err_system_generic";
    break;
  case _Z_ERR_SYSTEM_TASK_FAILED:
    reason = "_z_err_system_task_failed";
    break;
  case _Z_ERR_SYSTEM_OUT_OF_MEMORY:
    reason = "_z_err_system_out_of_memory";
    break;
  case _Z_ERR_CONNECTION_CLOSED:
    reason = "_z_err_connection_closed";
    break;
  case _Z_ERR_DID_NOT_READ:
    reason = "_z_err_did_not_read";
    break;
  case Z_EINVAL:
    reason = "z_einval";
    break;
  case _Z_ERR_OVERFLOW:
    reason = "_z_err_overflow";
    break;
  case _Z_ERR_SESSION_CLOSED:
    reason = "_z_err_session_closed";
    break;
  case Z_EDESERIALIZE:
    reason = "z_edeserialize";
    break;
  case Z_ETIMEDOUT:
    reason = "z_etimedout";
    break;
  case _Z_ERR_KEYEXPR_DECLARED_ON_ANOTHER_SESSION:
    reason = "_z_err_keyexpr_declared_on_another_session";
    break;
  case Z_ERR_CANCELLED:
    reason = "z_err_cancelled";
    break;
  case _Z_ERR_NULL:
    reason = "_z_err_null";
    break;
  case _Z_ERR_GENERIC:
    reason = "_z_err_generic";
    break;
  default:
    // TODO output value to log
    reason = "_z_err_unknown";
    break;
  }

  return reason;
}

ERL_NIF_TERM zxp_error_binary_zp(ErlNifEnv *env, const char *file, int line, z_result_t ret)
{
  const char *reason = zxp_error_char_zp(ret);
  return zxp_error_binary(env, file, line, reason);
}

ERL_NIF_TERM zxp_error_tuple_zp(ErlNifEnv *env, const char *file, int line, z_result_t ret)
{
  ERL_NIF_TERM binary = zxp_error_binary_zp(env, file, line, ret);
  return enif_make_tuple2(env, error_atom, binary);
}

ERL_NIF_TERM zxp_test_raise(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  UNUSED(argc);
  UNUSED(argv);

  return zxp_raise(env, __FILE__, __LINE__, "raise");
}
