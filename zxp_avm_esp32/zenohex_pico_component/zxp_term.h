#include <nifs.h>
#include <stddef.h>
#include <zenoh-pico.h>

// primitive
extern term not_found_atom;
extern term nil_atom;
extern term timeout_atom;
extern term session_closed_atom;
extern term struct_atom;

//
extern term payload_atom;
extern term encoding_atom;
extern term attachment_atom;
extern term express_atom;
extern term key_expr_atom;
extern term timestamp_atom;

// query option
extern term query_timeout_atom;
extern term accept_replies_atom;
extern term consolidation_atom;
extern term target_atom;

// kind
extern term kind_atom;
extern term delete_atom;
extern term put_atom;

// module
extern term sample_module;
extern term reply_error_module;

// congestion control
extern term congestion_control_atom;
extern term block_atom;
extern term drop_atom;

// consolidation
extern term auto_atom;
extern term none_atom;
extern term monotonic_atom;
extern term latest_atom;

// target
extern term best_matching_atom;
extern term all_atom;
extern term all_complete_atom;

// accept replies
extern term matching_query_atom;
extern term any_atom;

// priority
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

extern void zxp_init_atom(GlobalContext *global);
extern term zxp_raise(Context *ctx, const char *reason);
extern term zxp_binary_from_bytes(Context *ctx, const void *data, size_t size);
extern term zxp_make_tuple2(Context *ctx, term first, term second);
extern term zxp_map_from_arrays(Context *ctx, const term keys[], const term values[], size_t size);
extern term zxp_error_tuple_zp(Context *ctx, z_result_t ret);
extern term zxp_raise_zp(Context *ctx, z_result_t ret);
extern term zxp_test_raise(Context *ctx, int argc, term argv[]);
