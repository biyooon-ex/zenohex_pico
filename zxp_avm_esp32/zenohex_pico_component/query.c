#include <memory.h>
#include <stdlib.h>
#include <term.h>

#include "query.h"

static term reply_error_module_atom;
static term struct_atom;
static term payload_atom;
static term encoding_atom;

#define ZXP_ATOM(global, size, name) globalcontext_make_atom(global, ATOM_STR(size, name))

void zxp_query_init_atoms(GlobalContext *global)
{
  reply_error_module_atom = ZXP_ATOM(global, "\x23", "Elixir.ZenohexPico.Query.ReplyError");
  struct_atom = ZXP_ATOM(global, "\xA", "__struct__");
  payload_atom = ZXP_ATOM(global, "\x7", "payload");
  encoding_atom = ZXP_ATOM(global, "\x8", "encoding");
}

void zxp_reply_error_drop(zxp_reply_error_t *reply_error)
{
  free(reply_error->payload.data);
  free(reply_error->encoding.data);
}

size_t zxp_reply_error_heap_size(const zxp_reply_error_t *reply_error)
{
  return term_map_size_in_terms(3) +
      (reply_error->payload.data == NULL ? 0 : term_binary_heap_size(reply_error->payload.size)) +
      (reply_error->encoding.data == NULL ? 0 : term_binary_heap_size(reply_error->encoding.size));
}

static term zxp_bytes_to_term(Context *ctx, const zxp_bytes_t *bytes)
{
  return bytes->data == NULL ? term_nil() :
      term_from_literal_binary(bytes->data, bytes->size, &ctx->heap, ctx->global);
}

term zxp_reply_error_to_term(Context *ctx, const zxp_reply_error_t *reply_error)
{
  term result = term_alloc_map(3, &ctx->heap);
  term_set_map_assoc(result, 0, struct_atom, reply_error_module_atom);
  term_set_map_assoc(result, 1, payload_atom, zxp_bytes_to_term(ctx, &reply_error->payload));
  term_set_map_assoc(result, 2, encoding_atom, zxp_bytes_to_term(ctx, &reply_error->encoding));
  return result;
}
