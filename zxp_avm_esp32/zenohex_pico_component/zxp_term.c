#include <defaultatoms.h>
#include <memory.h>
#include <term.h>
#include <zenoh-pico.h>

#include "zxp_term.h"

// primitive
term not_found_atom;
term nil_atom;
term timeout_atom;
term session_closed_atom;
term struct_atom;

//
term payload_atom;
term encoding_atom;
term attachment_atom;
term express_atom;
term key_expr_atom;
term timestamp_atom;

// query option
term query_timeout_atom;
term accept_replies_atom;
term consolidation_atom;
term target_atom;

// kind
term kind_atom;
term delete_atom;
term put_atom;

// module
term sample_module;
term reply_error_module;

// congestion control
term congestion_control_atom;
term block_atom;
term drop_atom;

// consolidation
term auto_atom;
term none_atom;
term monotonic_atom;
term latest_atom;

// target
term best_matching_atom;
term all_atom;
term all_complete_atom;

// accept replies
term matching_query_atom;
term any_atom;

// priority
term priority_atom;
term real_time_atom;
term interactive_high_atom;
term interactive_low_atom;
term data_high_atom;
term data_atom;
term data_low_atom;
term background_atom;

void zxp_init_atom(GlobalContext *global)
{
  // primitive
  nil_atom = globalcontext_make_atom(global, ATOM_STR("\x3", "nil"));
  not_found_atom = globalcontext_make_atom(global, ATOM_STR("\x9", "not_found"));
  timeout_atom = globalcontext_make_atom(global, ATOM_STR("\x7", "timeout"));
  session_closed_atom = globalcontext_make_atom(global, ATOM_STR("\xE", "session_closed"));
  struct_atom = globalcontext_make_atom(global, ATOM_STR("\xA", "__struct__"));

  //
  payload_atom = globalcontext_make_atom(global, ATOM_STR("\x7", "payload"));
  encoding_atom = globalcontext_make_atom(global, ATOM_STR("\x8", "encoding"));
  attachment_atom = globalcontext_make_atom(global, ATOM_STR("\xA", "attachment"));
  express_atom = globalcontext_make_atom(global, ATOM_STR("\x7", "express"));
  key_expr_atom = globalcontext_make_atom(global, ATOM_STR("\x8", "key_expr"));
  timestamp_atom = globalcontext_make_atom(global, ATOM_STR("\x9", "timestamp"));

  // query option
  query_timeout_atom = globalcontext_make_atom(global, ATOM_STR("\xD", "query_timeout"));
  accept_replies_atom = globalcontext_make_atom(global, ATOM_STR("\xE", "accept_replies"));
  consolidation_atom = globalcontext_make_atom(global, ATOM_STR("\xD", "consolidation"));
  target_atom = globalcontext_make_atom(global, ATOM_STR("\x6", "target"));

  // kind
  kind_atom = globalcontext_make_atom(global, ATOM_STR("\x4", "kind"));
  delete_atom = globalcontext_make_atom(global, ATOM_STR("\x6", "delete"));
  put_atom = globalcontext_make_atom(global, ATOM_STR("\x3", "put"));

  // module
  sample_module = globalcontext_make_atom(global, ATOM_STR("\x19", "Elixir.ZenohexPico.Sample"));
  reply_error_module =
      globalcontext_make_atom(global, ATOM_STR("\x23", "Elixir.ZenohexPico.Query.ReplyError"));

  // congestion control
  congestion_control_atom = globalcontext_make_atom(global, ATOM_STR("\x12", "congestion_control"));
  block_atom = globalcontext_make_atom(global, ATOM_STR("\x5", "block"));
  drop_atom = globalcontext_make_atom(global, ATOM_STR("\x4", "drop"));

  // consolidation
  auto_atom = globalcontext_make_atom(global, ATOM_STR("\x4", "auto"));
  none_atom = globalcontext_make_atom(global, ATOM_STR("\x4", "none"));
  monotonic_atom = globalcontext_make_atom(global, ATOM_STR("\x9", "monotonic"));
  latest_atom = globalcontext_make_atom(global, ATOM_STR("\x6", "latest"));

  // target
  best_matching_atom = globalcontext_make_atom(global, ATOM_STR("\xD", "best_matching"));
  all_atom = globalcontext_make_atom(global, ATOM_STR("\x3", "all"));
  all_complete_atom = globalcontext_make_atom(global, ATOM_STR("\xC", "all_complete"));

  // accept replies
  matching_query_atom = globalcontext_make_atom(global, ATOM_STR("\xE", "matching_query"));
  any_atom = globalcontext_make_atom(global, ATOM_STR("\x3", "any"));

  // priority
  priority_atom = globalcontext_make_atom(global, ATOM_STR("\x8", "priority"));
  real_time_atom = globalcontext_make_atom(global, ATOM_STR("\x9", "real_time"));
  interactive_high_atom = globalcontext_make_atom(global, ATOM_STR("\x10", "interactive_high"));
  interactive_low_atom = globalcontext_make_atom(global, ATOM_STR("\xF", "interactive_low"));
  data_high_atom = globalcontext_make_atom(global, ATOM_STR("\x9", "data_high"));
  data_atom = globalcontext_make_atom(global, ATOM_STR("\x4", "data"));
  data_low_atom = globalcontext_make_atom(global, ATOM_STR("\x8", "data_low"));
  background_atom = globalcontext_make_atom(global, ATOM_STR("\xA", "background"));
}

term zxp_raise(Context *ctx, const char *reason)
{
  size_t reason_size = strlen(reason);

  if (UNLIKELY(memory_ensure_free(ctx, term_binary_heap_size(reason_size)) != MEMORY_GC_OK))
  {
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  term binary = term_from_literal_binary(reason, reason_size, &ctx->heap, ctx->global);
  RAISE_ERROR(binary);
}

term zxp_binary_from_bytes(Context *ctx, const void *data, size_t size)
{
  return term_from_literal_binary(data, size, &ctx->heap, ctx->global);
}

term zxp_make_tuple2(Context *ctx, term first, term second)
{
  term tuple = term_alloc_tuple(2, &ctx->heap);
  term_put_tuple_element(tuple, 0, first);
  term_put_tuple_element(tuple, 1, second);
  return tuple;
}

term zxp_map_from_arrays(Context *ctx, const term keys[], const term values[], size_t size)
{
  term result = term_alloc_map(size, &ctx->heap);
  for (size_t index = 0; index < size; index++)
  {
    term_set_map_assoc(result, index, keys[index], values[index]);
  }
  return result;
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

term zxp_error_tuple_zp(Context *ctx, z_result_t ret)
{
  const char *reason = zxp_error_char_zp(ret);
  size_t reason_size = strlen(reason);

  if (UNLIKELY(memory_ensure_free(ctx, term_binary_heap_size(reason_size) + TUPLE_SIZE(2)) !=
               MEMORY_GC_OK))
  {
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  term binary = term_from_literal_binary(reason, reason_size, &ctx->heap, ctx->global);
  return zxp_make_tuple2(ctx, ERROR_ATOM, binary);
}

term zxp_raise_zp(Context *ctx, z_result_t ret)
{
  const char *reason = zxp_error_char_zp(ret);
  return zxp_raise(ctx, reason);
}

term zxp_test_raise(Context *ctx, int argc, term argv[])
{
  UNUSED(argc);
  UNUSED(argv);

  return zxp_raise(ctx, "raise");
}
