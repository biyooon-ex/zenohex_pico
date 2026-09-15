#include <term.h>

#include "avm_compat.h"
#include "query.h"

static bool zxp_binary_heap_size_from_zp_bytes(const z_loaned_bytes_t *bytes, size_t *heap_size)
{
  if (bytes == NULL)
  {
    *heap_size = 0;
    return true;
  }

  z_owned_slice_t slice;
  z_internal_null(&slice);
  z_result_t ret = z_bytes_to_slice(bytes, &slice);
  if (ret == Z_OK)
  {
    *heap_size = term_binary_heap_size(z_slice_len(z_loan(slice)));
  }
  z_drop(z_move(slice));
  return ret == Z_OK;
}

static bool zxp_binary_heap_size_from_zp_encoding(const z_loaned_encoding_t *encoding,
                                                  size_t *heap_size)
{
  if (encoding == NULL)
  {
    *heap_size = 0;
    return true;
  }

  z_owned_string_t string;
  z_internal_null(&string);
  z_result_t ret = z_encoding_to_string(encoding, &string);
  if (ret == Z_OK)
  {
    *heap_size = term_binary_heap_size(z_string_len(z_loan(string)));
  }
  z_drop(z_move(string));
  return ret == Z_OK;
}

bool zxp_reply_error_heap_size(const z_loaned_reply_err_t *reply_error, size_t *heap_size)
{
  size_t payload_heap_size;
  size_t encoding_heap_size;
  if (!zxp_binary_heap_size_from_zp_bytes(z_reply_err_payload(reply_error), &payload_heap_size) ||
      !zxp_binary_heap_size_from_zp_encoding(z_reply_err_encoding(reply_error),
                                             &encoding_heap_size))
  {
    return false;
  }
  *heap_size = term_map_size_in_terms(3) + payload_heap_size + encoding_heap_size;
  return true;
}

static term zxp_binary_from_zp_bytes(Context *ctx, const z_loaned_bytes_t *bytes)
{
  if (bytes == NULL)
  {
    return nil_atom;
  }

  z_owned_slice_t slice;
  z_internal_null(&slice);
  term term;
  {
    z_result_t ret = z_bytes_to_slice(bytes, &slice);
    if (ret != Z_OK)
    {
      return zxp_raise_zp(ctx, ret);
    }

    term = zxp_binary_from_bytes(ctx, z_slice_data(z_loan(slice)), z_slice_len(z_loan(slice)));
  }
  z_drop(z_move(slice));
  return term;
}

static term zxp_binary_from_zp_encoding(Context *ctx, const z_loaned_encoding_t *encoding)
{
  if (encoding == NULL)
  {
    return nil_atom;
  }

  z_owned_string_t string;
  z_internal_null(&string);
  term term;
  {
    z_result_t ret = z_encoding_to_string(encoding, &string);
    if (ret != Z_OK)
    {
      return zxp_raise_zp(ctx, ret);
    }

    term = zxp_binary_from_bytes(ctx, z_string_data(z_loan(string)), z_string_len(z_loan(string)));
  }
  z_drop(z_move(string));
  return term;
}

term zxp_struct_from_zp_reply_err(Context *ctx, const z_loaned_reply_err_t *reply_err)
{
  term keys[] = {
      struct_atom,
      payload_atom,
      encoding_atom,
  };

  term values[] = {
      reply_error_module,
      zxp_binary_from_zp_bytes(ctx, z_reply_err_payload(reply_err)),
      zxp_binary_from_zp_encoding(ctx, z_reply_err_encoding(reply_err)),
  };

  for (size_t index = 0; index < 3; index++)
  {
    if (term_is_invalid_term(values[index]))
    {
      return term_invalid_term();
    }
  }

  term term;
  term = zxp_map_from_arrays(ctx, keys, values, 3);
  return term;
}
