#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <term.h>

#include "avm_compat.h"
#include "query.h"

static bool zxp_owned_bytes_from_zp_bytes(zxp_bytes_t *destination, const z_loaned_bytes_t *bytes)
{
  destination->data = NULL;
  destination->size = 0;
  if (bytes == NULL)
  {
    return true;
  }

  z_owned_slice_t slice;
  z_internal_null(&slice);
  {
    z_result_t ret = z_bytes_to_slice(bytes, &slice);
    if (ret != Z_OK)
    {
      return false;
    }
  }
  destination->size = z_slice_len(z_loan(slice));
  destination->data = malloc(destination->size == 0 ? 1 : destination->size);
  if (destination->data != NULL && destination->size != 0)
  {
    memcpy(destination->data, z_slice_data(z_loan(slice)), destination->size);
  }
  z_drop(z_move(slice));
  return destination->data != NULL;
}

static bool zxp_owned_bytes_from_zp_encoding(zxp_bytes_t *destination,
                                             const z_loaned_encoding_t *encoding)
{
  destination->data = NULL;
  destination->size = 0;
  if (encoding == NULL)
  {
    return true;
  }

  z_owned_string_t string;
  z_internal_null(&string);
  {
    z_result_t ret = z_encoding_to_string(encoding, &string);
    if (ret != Z_OK)
    {
      return false;
    }
  }
  destination->size = z_string_len(z_loan(string));
  destination->data = malloc(destination->size == 0 ? 1 : destination->size);
  if (destination->data != NULL && destination->size != 0)
  {
    memcpy(destination->data, z_string_data(z_loan(string)), destination->size);
  }
  z_drop(z_move(string));
  return destination->data != NULL;
}

void zxp_reply_error_drop(zxp_reply_error_t *reply_error)
{
  free(reply_error->payload.data);
  free(reply_error->encoding.data);
}

bool zxp_reply_error_from_zp_reply_err(zxp_reply_error_t *destination,
                                       const z_loaned_reply_err_t *reply_err)
{
  memset(destination, 0, sizeof(*destination));
  if (!zxp_owned_bytes_from_zp_bytes(&destination->payload, z_reply_err_payload(reply_err)) ||
      !zxp_owned_bytes_from_zp_encoding(&destination->encoding, z_reply_err_encoding(reply_err)))
  {
    zxp_reply_error_drop(destination);
    return false;
  }
  return true;
}

size_t zxp_reply_error_heap_size(const zxp_reply_error_t *reply_error)
{
  return term_map_size_in_terms(3) +
         (reply_error->payload.data == NULL ? 0
                                            : term_binary_heap_size(reply_error->payload.size)) +
         (reply_error->encoding.data == NULL ? 0
                                             : term_binary_heap_size(reply_error->encoding.size));
}

static term zxp_bytes_to_term(Context *ctx, const zxp_bytes_t *bytes)
{
  return bytes->data == NULL ? nil_atom : zxp_avm_binary_from_bytes(ctx, bytes->data, bytes->size);
}

term zxp_struct_from_zp_reply_err(Context *ctx, const zxp_reply_error_t *reply_err)
{
  term keys[] = {struct_atom, payload_atom, encoding_atom};
  term values[] = {
      reply_error_module,
      zxp_bytes_to_term(ctx, &reply_err->payload),
      zxp_bytes_to_term(ctx, &reply_err->encoding),
  };
  return zxp_avm_map_from_arrays(ctx, keys, values, 3);
}
