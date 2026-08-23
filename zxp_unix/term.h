#include <erl_nif.h>
#include <zenoh-pico.h>

// primitive
extern ERL_NIF_TERM ok_atom;
extern ERL_NIF_TERM error_atom;
extern ERL_NIF_TERM not_found_atom;
extern ERL_NIF_TERM nil_atom;
extern ERL_NIF_TERM timeout_atom;
extern ERL_NIF_TERM struct_atom;
extern ERL_NIF_TERM true_atom;
extern ERL_NIF_TERM false_atom;

//
extern ERL_NIF_TERM payload_atom;
extern ERL_NIF_TERM encoding_atom;
extern ERL_NIF_TERM attachment_atom;
extern ERL_NIF_TERM express_atom;
extern ERL_NIF_TERM key_expr_atom;
extern ERL_NIF_TERM timestamp_atom;

// query option
extern ERL_NIF_TERM query_timeout_atom;
extern ERL_NIF_TERM accept_replies_atom;
extern ERL_NIF_TERM allowed_destination_atom;
extern ERL_NIF_TERM allowed_origin_atom;
extern ERL_NIF_TERM consolidation_atom;
extern ERL_NIF_TERM target_atom;

// kind
extern ERL_NIF_TERM kind_atom;
extern ERL_NIF_TERM delete_atom;
extern ERL_NIF_TERM put_atom;

// module
extern ERL_NIF_TERM sample_module;
extern ERL_NIF_TERM reply_error_module;

// congestion control
extern ERL_NIF_TERM congestion_control_atom;
extern ERL_NIF_TERM block_atom;
extern ERL_NIF_TERM drop_atom;

// consolidation
extern ERL_NIF_TERM auto_atom;
extern ERL_NIF_TERM none_atom;
extern ERL_NIF_TERM monotonic_atom;
extern ERL_NIF_TERM latest_atom;

// target
extern ERL_NIF_TERM best_matching_atom;
extern ERL_NIF_TERM all_atom;
extern ERL_NIF_TERM all_complete_atom;

// accept replies
extern ERL_NIF_TERM matching_query_atom;
extern ERL_NIF_TERM any_atom;

// allowed destination
extern ERL_NIF_TERM session_local_atom;
extern ERL_NIF_TERM remote_atom;

// priority
extern ERL_NIF_TERM priority_atom;
extern ERL_NIF_TERM real_time_atom;
extern ERL_NIF_TERM interactive_high_atom;
extern ERL_NIF_TERM interactive_low_atom;
extern ERL_NIF_TERM data_high_atom;
extern ERL_NIF_TERM data_atom;
extern ERL_NIF_TERM data_low_atom;
extern ERL_NIF_TERM background_atom;

extern void zxp_init_atom(ErlNifEnv *env);
extern ERL_NIF_TERM zxp_raise(ErlNifEnv *env, const char *file, int line, const char *reason);
extern ERL_NIF_TERM zxp_raise_null_pointer(ErlNifEnv *env, const char *file, int line);
extern ERL_NIF_TERM zxp_error_tuple_zp(ErlNifEnv *env, const char *file, int line, z_result_t ret);
extern ERL_NIF_TERM zxp_error_binary_zp(ErlNifEnv *env, const char *file, int line, z_result_t ret);
extern const char *zxp_error_char_zp(z_result_t ret);
extern ERL_NIF_TERM zxp_test_raise(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[]);
