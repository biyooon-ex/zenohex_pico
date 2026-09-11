#ifndef ZXP_AVM_COMPAT_H
#define ZXP_AVM_COMPAT_H

#include <nifs.h>
#include <stddef.h>

// Primitive.
extern term nil_atom;
extern term not_found_atom;
extern term timeout_atom;
extern term session_closed_atom;
extern term struct_atom;

// Sample fields.
extern term payload_atom;
extern term encoding_atom;
extern term attachment_atom;
extern term express_atom;
extern term key_expr_atom;
extern term timestamp_atom;

// Query options.
extern term query_timeout_atom;
extern term accept_replies_atom;
extern term consolidation_atom;
extern term target_atom;

// Sample kind.
extern term kind_atom;
extern term delete_atom;
extern term put_atom;

// Modules.
extern term sample_module;
extern term reply_error_module;

// Congestion control.
extern term congestion_control_atom;
extern term block_atom;
extern term drop_atom;

// Consolidation.
extern term auto_atom;
extern term none_atom;
extern term monotonic_atom;
extern term latest_atom;

// Query target.
extern term best_matching_atom;
extern term all_atom;
extern term all_complete_atom;

// Accepted replies.
extern term matching_query_atom;
extern term any_atom;

// Priority.
extern term priority_atom;
extern term real_time_atom;
extern term interactive_high_atom;
extern term interactive_low_atom;
extern term data_high_atom;
extern term data_atom;
extern term data_low_atom;
extern term background_atom;

static inline term zxp_avm_empty_list(void)
{
  return term_nil();
}

extern void zxp_avm_init_atoms(GlobalContext *global);
extern term zxp_binary_from_bytes(Context *ctx, const void *data, size_t size);
extern term zxp_make_tuple2(Context *ctx, term first, term second);
term zxp_avm_map_from_arrays(Context *ctx, const term keys[], const term values[], size_t size);
extern term zxp_avm_error_tuple(Context *ctx, const char *reason);

#endif
