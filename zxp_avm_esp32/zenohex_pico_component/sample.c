#include <defaultatoms.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <term.h>

#include "avm_compat.h"
#include "sample.h"
#include "timestamp.h"

static bool zxp_bytes_heap_size(const z_loaned_bytes_t *bytes, size_t *heap_size)
{
  if (bytes == NULL)
  {
    return true;
  }

  z_owned_slice_t slice;
  z_internal_null(&slice);
  if (z_bytes_to_slice(bytes, &slice) != Z_OK)
  {
    return false;
  }
  *heap_size += term_binary_heap_size(z_slice_len(z_loan(slice)));
  z_drop(z_move(slice));
  return true;
}

static bool zxp_encoding_heap_size(const z_loaned_encoding_t *encoding, size_t *heap_size)
{
  if (encoding == NULL)
  {
    return true;
  }

  z_owned_string_t string;
  z_internal_null(&string);
  z_result_t ret = z_encoding_to_string(encoding, &string);
  if (ret != Z_OK)
  {
    return false;
  }
  *heap_size += term_binary_heap_size(z_string_len(z_loan(string)));
  z_drop(z_move(string));
  return true;
}

static bool zxp_keyexpr_heap_size(const z_loaned_keyexpr_t *keyexpr, size_t *heap_size)
{
  z_view_string_t string;
  if (z_keyexpr_as_view_string(keyexpr, &string) != Z_OK)
  {
    return false;
  }
  *heap_size += term_binary_heap_size(z_string_len(z_loan(string)));
  return true;
}

static bool zxp_timestamp_heap_size(const z_timestamp_t *timestamp, size_t *heap_size)
{
  if (timestamp != NULL)
  {
    *heap_size += term_binary_heap_size(63);
  }
  return true;
}

bool zxp_sample_heap_size(const z_loaned_sample_t *sample, size_t *heap_size)
{
  *heap_size = term_map_size_in_terms(10);
  if (!zxp_bytes_heap_size(z_sample_attachment(sample), heap_size) ||
      !zxp_encoding_heap_size(z_sample_encoding(sample), heap_size) ||
      !zxp_keyexpr_heap_size(z_sample_keyexpr(sample), heap_size) ||
      !zxp_bytes_heap_size(z_sample_payload(sample), heap_size) ||
      !zxp_timestamp_heap_size(z_sample_timestamp(sample), heap_size))
  {
    return false;
  }
  return true;
}

static term zxp_atom_from_zp_congestion_control(z_congestion_control_t congestion_control)
{
  switch (congestion_control)
  {
  case Z_CONGESTION_CONTROL_DROP:
    return drop_atom;
  case Z_CONGESTION_CONTROL_BLOCK:
    return block_atom;
  default:
    return drop_atom;
  }
}

static term zxp_atom_from_zp_express(bool express)
{
  return express ? TRUE_ATOM : FALSE_ATOM;
}

static term zxp_atom_from_zp_sample_kind(z_sample_kind_t kind)
{
  switch (kind)
  {
  case Z_SAMPLE_KIND_PUT:
    return put_atom;
  case Z_SAMPLE_KIND_DELETE:
    return delete_atom;
  default:
    return put_atom;
  }
}

static term zxp_atom_from_zp_priority(z_priority_t priority)
{
  switch (priority)
  {
  case Z_PRIORITY_REAL_TIME:
    return real_time_atom;
  case Z_PRIORITY_INTERACTIVE_HIGH:
    return interactive_high_atom;
  case Z_PRIORITY_INTERACTIVE_LOW:
    return interactive_low_atom;
  case Z_PRIORITY_DATA_HIGH:
    return data_high_atom;
  case Z_PRIORITY_DATA:
    return data_atom;
  case Z_PRIORITY_DATA_LOW:
    return data_low_atom;
  case Z_PRIORITY_BACKGROUND:
    return background_atom;
  default:
    return data_atom;
  }
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
      RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    }
    term = zxp_binary_from_bytes(ctx, z_slice_data(z_loan(slice)), z_slice_len(z_loan(slice)));
  }
  z_drop(z_move(slice));
  return term;
}

static term zxp_binary_from_zp_keyexpr(Context *ctx, const z_loaned_keyexpr_t *keyexpr)
{
  z_view_string_t string;
  term term;
  {
    z_result_t ret = z_keyexpr_as_view_string(keyexpr, &string);
    if (ret != Z_OK)
    {
      RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    }

    term = zxp_binary_from_bytes(ctx, z_string_data(z_loan(string)), z_string_len(z_loan(string)));
  }
  return term;
}

static term zxp_binary_from_zp_encoding(Context *ctx, const z_loaned_encoding_t *encoding)
{
  z_owned_string_t string;
  z_internal_null(&string);
  term term;
  {
    z_result_t ret = z_encoding_to_string(encoding, &string);
    if (ret != Z_OK)
    {
      RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    }

    term = zxp_binary_from_bytes(ctx, z_string_data(z_loan(string)), z_string_len(z_loan(string)));
  }
  z_drop(z_move(string));
  return term;
}

term zxp_struct_from_zp_sample(Context *ctx, const z_loaned_sample_t *sample)
{
  term keys[] = {
      struct_atom,
      attachment_atom,
      congestion_control_atom,
      encoding_atom,
      express_atom,
      key_expr_atom,
      kind_atom,
      payload_atom,
      priority_atom,
      timestamp_atom,
  };

  term values[] = {
      sample_module,
      zxp_binary_from_zp_bytes(ctx, z_sample_attachment(sample)),
      zxp_atom_from_zp_congestion_control(z_sample_congestion_control(sample)),
      zxp_binary_from_zp_encoding(ctx, z_sample_encoding(sample)),
      zxp_atom_from_zp_express(z_sample_express(sample)),
      zxp_binary_from_zp_keyexpr(ctx, z_sample_keyexpr(sample)),
      zxp_atom_from_zp_sample_kind(z_sample_kind(sample)),
      zxp_binary_from_zp_bytes(ctx, z_sample_payload(sample)),
      zxp_atom_from_zp_priority(z_sample_priority(sample)),
      zxp_binary_from_zp_timestamp(ctx, z_sample_timestamp(sample)),
  };

  for (size_t index = 0; index < 10; index++)
  {
    if (term_is_invalid_term(values[index]))
    {
      return term_invalid_term();
    }
  }

  term term;
  term = zxp_map_from_arrays(ctx, keys, values, 10);
  return term;
}
