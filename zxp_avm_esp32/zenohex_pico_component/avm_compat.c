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

static term zxp_error_binary(Context *ctx, const char *reason)
{
  size_t reason_size = strlen(reason);

  if (UNLIKELY(memory_ensure_free(ctx, term_binary_heap_size(reason_size)) != MEMORY_GC_OK))
  {
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  return term_from_literal_binary(reason, reason_size, &ctx->heap, ctx->global);
}

void zxp_init_atom(GlobalContext *global)
{
  // Primitive.
  nil_atom = globalcontext_make_atom(global, ATOM_STR("\x3", "nil"));
  not_found_atom = globalcontext_make_atom(global, ATOM_STR("\x9", "not_found"));
  timeout_atom = globalcontext_make_atom(global, ATOM_STR("\x7", "timeout"));
  session_closed_atom = globalcontext_make_atom(global, ATOM_STR("\xE", "session_closed"));
  struct_atom = globalcontext_make_atom(global, ATOM_STR("\xA", "__struct__"));

  // Sample fields.
  payload_atom = globalcontext_make_atom(global, ATOM_STR("\x7", "payload"));
  encoding_atom = globalcontext_make_atom(global, ATOM_STR("\x8", "encoding"));
  attachment_atom = globalcontext_make_atom(global, ATOM_STR("\xA", "attachment"));
  express_atom = globalcontext_make_atom(global, ATOM_STR("\x7", "express"));
  key_expr_atom = globalcontext_make_atom(global, ATOM_STR("\x8", "key_expr"));
  timestamp_atom = globalcontext_make_atom(global, ATOM_STR("\x9", "timestamp"));

  // Query options.
  query_timeout_atom = globalcontext_make_atom(global, ATOM_STR("\xD", "query_timeout"));
  accept_replies_atom = globalcontext_make_atom(global, ATOM_STR("\xE", "accept_replies"));
  consolidation_atom = globalcontext_make_atom(global, ATOM_STR("\xD", "consolidation"));
  target_atom = globalcontext_make_atom(global, ATOM_STR("\x6", "target"));

  // Sample kind.
  kind_atom = globalcontext_make_atom(global, ATOM_STR("\x4", "kind"));
  delete_atom = globalcontext_make_atom(global, ATOM_STR("\x6", "delete"));
  put_atom = globalcontext_make_atom(global, ATOM_STR("\x3", "put"));

  // Modules.
  sample_module = globalcontext_make_atom(global, ATOM_STR("\x19", "Elixir.ZenohexPico.Sample"));
  reply_error_module =
      globalcontext_make_atom(global, ATOM_STR("\x23", "Elixir.ZenohexPico.Query.ReplyError"));

  // Congestion control.
  congestion_control_atom = globalcontext_make_atom(global, ATOM_STR("\x12", "congestion_control"));
  block_atom = globalcontext_make_atom(global, ATOM_STR("\x5", "block"));
  drop_atom = globalcontext_make_atom(global, ATOM_STR("\x4", "drop"));

  // Consolidation.
  auto_atom = globalcontext_make_atom(global, ATOM_STR("\x4", "auto"));
  none_atom = globalcontext_make_atom(global, ATOM_STR("\x4", "none"));
  monotonic_atom = globalcontext_make_atom(global, ATOM_STR("\x9", "monotonic"));
  latest_atom = globalcontext_make_atom(global, ATOM_STR("\x6", "latest"));

  // Query target.
  best_matching_atom = globalcontext_make_atom(global, ATOM_STR("\xD", "best_matching"));
  all_atom = globalcontext_make_atom(global, ATOM_STR("\x3", "all"));
  all_complete_atom = globalcontext_make_atom(global, ATOM_STR("\xC", "all_complete"));

  // Accepted replies.
  matching_query_atom = globalcontext_make_atom(global, ATOM_STR("\xE", "matching_query"));
  any_atom = globalcontext_make_atom(global, ATOM_STR("\x3", "any"));

  // Priority.
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
  RAISE_ERROR(zxp_error_binary(ctx, reason));
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

term zxp_test_raise(Context *ctx, int argc, term argv[])
{
  UNUSED(argc);
  UNUSED(argv);

  return zxp_raise(ctx, "raise");
}
