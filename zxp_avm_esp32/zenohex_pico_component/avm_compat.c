#include <defaultatoms.h>
#include <memory.h>
#include <term.h>

#include "avm_compat.h"

// Primitive.
term nil_atom;
term not_found_atom;
term timeout_atom;
term session_closed_atom;
term struct_atom;

// Sample fields.
term payload_atom;
term encoding_atom;
term attachment_atom;
term express_atom;
term key_expr_atom;
term timestamp_atom;

// Query options.
term query_timeout_atom;
term accept_replies_atom;
term consolidation_atom;
term target_atom;

// Sample kind.
term kind_atom;
term delete_atom;
term put_atom;

// Modules.
term sample_module;
term reply_error_module;

// Congestion control.
term congestion_control_atom;
term block_atom;
term drop_atom;

// Consolidation.
term auto_atom;
term none_atom;
term monotonic_atom;
term latest_atom;

// Query target.
term best_matching_atom;
term all_atom;
term all_complete_atom;

// Accepted replies.
term matching_query_atom;
term any_atom;

// Priority.
term priority_atom;
term real_time_atom;
term interactive_high_atom;
term interactive_low_atom;
term data_high_atom;
term data_atom;
term data_low_atom;
term background_atom;

#define ZXP_ATOM(global, size, name) globalcontext_make_atom(global, ATOM_STR(size, name))

void zxp_avm_init_atoms(GlobalContext *global)
{
  // Primitive.
  nil_atom = ZXP_ATOM(global, "\x3", "nil");
  not_found_atom = ZXP_ATOM(global, "\x9", "not_found");
  timeout_atom = ZXP_ATOM(global, "\x7", "timeout");
  session_closed_atom = ZXP_ATOM(global, "\xE", "session_closed");
  struct_atom = ZXP_ATOM(global, "\xA", "__struct__");

  // Sample fields.
  payload_atom = ZXP_ATOM(global, "\x7", "payload");
  encoding_atom = ZXP_ATOM(global, "\x8", "encoding");
  attachment_atom = ZXP_ATOM(global, "\xA", "attachment");
  express_atom = ZXP_ATOM(global, "\x7", "express");
  key_expr_atom = ZXP_ATOM(global, "\x8", "key_expr");
  timestamp_atom = ZXP_ATOM(global, "\x9", "timestamp");

  // Query options.
  query_timeout_atom = ZXP_ATOM(global, "\xD", "query_timeout");
  accept_replies_atom = ZXP_ATOM(global, "\xE", "accept_replies");
  consolidation_atom = ZXP_ATOM(global, "\xD", "consolidation");
  target_atom = ZXP_ATOM(global, "\x6", "target");

  // Sample kind.
  kind_atom = ZXP_ATOM(global, "\x4", "kind");
  delete_atom = ZXP_ATOM(global, "\x6", "delete");
  put_atom = ZXP_ATOM(global, "\x3", "put");

  // Modules.
  sample_module = ZXP_ATOM(global, "\x19", "Elixir.ZenohexPico.Sample");
  reply_error_module = ZXP_ATOM(global, "\x23", "Elixir.ZenohexPico.Query.ReplyError");

  // Congestion control.
  congestion_control_atom = ZXP_ATOM(global, "\x12", "congestion_control");
  block_atom = ZXP_ATOM(global, "\x5", "block");
  drop_atom = ZXP_ATOM(global, "\x4", "drop");

  // Consolidation.
  auto_atom = ZXP_ATOM(global, "\x4", "auto");
  none_atom = ZXP_ATOM(global, "\x4", "none");
  monotonic_atom = ZXP_ATOM(global, "\x9", "monotonic");
  latest_atom = ZXP_ATOM(global, "\x6", "latest");

  // Query target.
  best_matching_atom = ZXP_ATOM(global, "\xD", "best_matching");
  all_atom = ZXP_ATOM(global, "\x3", "all");
  all_complete_atom = ZXP_ATOM(global, "\xC", "all_complete");

  // Accepted replies.
  matching_query_atom = ZXP_ATOM(global, "\xE", "matching_query");
  any_atom = ZXP_ATOM(global, "\x3", "any");

  // Priority.
  priority_atom = ZXP_ATOM(global, "\x8", "priority");
  real_time_atom = ZXP_ATOM(global, "\x9", "real_time");
  interactive_high_atom = ZXP_ATOM(global, "\x10", "interactive_high");
  interactive_low_atom = ZXP_ATOM(global, "\xF", "interactive_low");
  data_high_atom = ZXP_ATOM(global, "\x9", "data_high");
  data_atom = ZXP_ATOM(global, "\x4", "data");
  data_low_atom = ZXP_ATOM(global, "\x8", "data_low");
  background_atom = ZXP_ATOM(global, "\xA", "background");
}

term zxp_binary_from_bytes(Context *ctx, const void *data, size_t size)
{
  return term_from_literal_binary(data, size, &ctx->heap, ctx->global);
}

term zxp_make_tuple2(Context *ctx, term first, term second)
{
  term result = term_alloc_tuple(2, &ctx->heap);
  term_put_tuple_element(result, 0, first);
  term_put_tuple_element(result, 1, second);
  return result;
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

term zxp_error_tuple(Context *ctx, const char *reason)
{
  size_t reason_size = strlen(reason);
  if (memory_ensure_free(ctx, term_binary_heap_size(reason_size) + TUPLE_SIZE(2)) != MEMORY_GC_OK)
  {
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }
  return zxp_make_tuple2(ctx, ERROR_ATOM, zxp_binary_from_bytes(ctx, reason, reason_size));
}
