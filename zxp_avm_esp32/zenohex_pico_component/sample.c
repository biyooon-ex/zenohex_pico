#include <defaultatoms.h>
#include <memory.h>
#include <stdlib.h>
#include <term.h>

#include "sample.h"
#include "timestamp.h"

static term sample_module_atom;
static term struct_atom;
static term attachment_atom;
static term congestion_control_atom;
static term encoding_atom;
static term express_atom;
static term key_expr_atom;
static term kind_atom;
static term payload_atom;
static term priority_atom;
static term timestamp_atom;
static term block_atom;
static term drop_atom;
static term put_atom;
static term delete_atom;
static term real_time_atom;
static term interactive_high_atom;
static term interactive_low_atom;
static term data_high_atom;
static term data_atom;
static term data_low_atom;
static term background_atom;

#define ZXP_ATOM(global, size, name) globalcontext_make_atom(global, ATOM_STR(size, name))

void zxp_sample_init_atoms(GlobalContext *global)
{
  sample_module_atom = ZXP_ATOM(global, "\x19", "Elixir.ZenohexPico.Sample");
  struct_atom = ZXP_ATOM(global, "\xA", "__struct__");
  attachment_atom = ZXP_ATOM(global, "\xA", "attachment");
  congestion_control_atom = ZXP_ATOM(global, "\x12", "congestion_control");
  encoding_atom = ZXP_ATOM(global, "\x8", "encoding");
  express_atom = ZXP_ATOM(global, "\x7", "express");
  key_expr_atom = ZXP_ATOM(global, "\x8", "key_expr");
  kind_atom = ZXP_ATOM(global, "\x4", "kind");
  payload_atom = ZXP_ATOM(global, "\x7", "payload");
  priority_atom = ZXP_ATOM(global, "\x8", "priority");
  timestamp_atom = ZXP_ATOM(global, "\x9", "timestamp");
  block_atom = ZXP_ATOM(global, "\x5", "block");
  drop_atom = ZXP_ATOM(global, "\x4", "drop");
  put_atom = ZXP_ATOM(global, "\x3", "put");
  delete_atom = ZXP_ATOM(global, "\x6", "delete");
  real_time_atom = ZXP_ATOM(global, "\x9", "real_time");
  interactive_high_atom = ZXP_ATOM(global, "\x10", "interactive_high");
  interactive_low_atom = ZXP_ATOM(global, "\xF", "interactive_low");
  data_high_atom = ZXP_ATOM(global, "\x9", "data_high");
  data_atom = ZXP_ATOM(global, "\x4", "data");
  data_low_atom = ZXP_ATOM(global, "\x8", "data_low");
  background_atom = ZXP_ATOM(global, "\xA", "background");
}

void zxp_sample_drop(zxp_sample_t *sample)
{
  free(sample->attachment.data);
  free(sample->encoding.data);
  free(sample->key_expr.data);
  free(sample->payload.data);
}

static size_t zxp_bytes_heap_size(const zxp_bytes_t *bytes)
{
  return bytes->data == NULL ? 0 : term_binary_heap_size(bytes->size);
}

size_t zxp_sample_heap_size(const zxp_sample_t *sample)
{
  return term_map_size_in_terms(10) + zxp_bytes_heap_size(&sample->attachment) +
      zxp_bytes_heap_size(&sample->encoding) + zxp_bytes_heap_size(&sample->key_expr) +
      zxp_bytes_heap_size(&sample->payload) + (sample->has_timestamp ? term_binary_heap_size(63) : 0);
}

static term zxp_bytes_to_term(Context *ctx, const zxp_bytes_t *bytes)
{
  return bytes->data == NULL ? term_nil() :
      term_from_literal_binary(bytes->data, bytes->size, &ctx->heap, ctx->global);
}

static term zxp_priority_to_atom(z_priority_t priority)
{
  switch (priority) {
    case Z_PRIORITY_REAL_TIME: return real_time_atom;
    case Z_PRIORITY_INTERACTIVE_HIGH: return interactive_high_atom;
    case Z_PRIORITY_INTERACTIVE_LOW: return interactive_low_atom;
    case Z_PRIORITY_DATA_HIGH: return data_high_atom;
    case Z_PRIORITY_DATA_LOW: return data_low_atom;
    case Z_PRIORITY_BACKGROUND: return background_atom;
    default: return data_atom;
  }
}

term zxp_sample_to_term(Context *ctx, const zxp_sample_t *sample)
{
  term result = term_alloc_map(10, &ctx->heap);
  term_set_map_assoc(result, 0, struct_atom, sample_module_atom);
  term_set_map_assoc(result, 1, attachment_atom, zxp_bytes_to_term(ctx, &sample->attachment));
  term_set_map_assoc(result, 2, congestion_control_atom,
      sample->congestion_control == Z_CONGESTION_CONTROL_BLOCK ? block_atom : drop_atom);
  term_set_map_assoc(result, 3, encoding_atom, zxp_bytes_to_term(ctx, &sample->encoding));
  term_set_map_assoc(result, 4, express_atom, sample->express ? TRUE_ATOM : FALSE_ATOM);
  term_set_map_assoc(result, 5, key_expr_atom, zxp_bytes_to_term(ctx, &sample->key_expr));
  term_set_map_assoc(result, 6, kind_atom, sample->kind == Z_SAMPLE_KIND_DELETE ? delete_atom : put_atom);
  term_set_map_assoc(result, 7, payload_atom, zxp_bytes_to_term(ctx, &sample->payload));
  term_set_map_assoc(result, 8, priority_atom, zxp_priority_to_atom(sample->priority));
  if (sample->has_timestamp) {
    char timestamp[63];
    if (!zxp_timestamp_to_binary(&sample->timestamp, timestamp)) RAISE_ERROR(BADARG_ATOM);
    term_set_map_assoc(result, 9, timestamp_atom,
        term_from_literal_binary(timestamp, sizeof(timestamp), &ctx->heap, ctx->global));
  } else {
    term_set_map_assoc(result, 9, timestamp_atom, term_nil());
  }
  return result;
}
