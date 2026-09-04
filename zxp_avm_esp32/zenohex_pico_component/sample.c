#include <defaultatoms.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <term.h>

#include "avm_compat.h"
#include "sample.h"
#include "timestamp.h"

void zxp_sample_drop(zxp_sample_t *sample)
{
  free(sample->attachment.data);
  free(sample->encoding.data);
  free(sample->keyexpr.data);
  free(sample->payload.data);
}

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

static bool zxp_owned_bytes_from_zp_keyexpr(zxp_bytes_t *destination,
                                            const z_loaned_keyexpr_t *keyexpr)
{
  destination->data = NULL;
  destination->size = 0;
  z_view_string_t string;
  {
    z_result_t ret = z_keyexpr_as_view_string(keyexpr, &string);
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
  return destination->data != NULL;
}

bool zxp_sample_from_zp_sample(zxp_sample_t *destination, const z_loaned_sample_t *sample)
{
  memset(destination, 0, sizeof(*destination));
  destination->congestion_control = z_sample_congestion_control(sample);
  destination->express = z_sample_express(sample);
  destination->kind = z_sample_kind(sample);
  destination->priority = z_sample_priority(sample);
  const z_timestamp_t *timestamp = z_sample_timestamp(sample);
  if (timestamp != NULL)
  {
    destination->has_timestamp = true;
    destination->timestamp = *timestamp;
  }
  if (!zxp_owned_bytes_from_zp_bytes(&destination->attachment, z_sample_attachment(sample)) ||
      !zxp_owned_bytes_from_zp_encoding(&destination->encoding, z_sample_encoding(sample)) ||
      !zxp_owned_bytes_from_zp_keyexpr(&destination->keyexpr, z_sample_keyexpr(sample)) ||
      !zxp_owned_bytes_from_zp_bytes(&destination->payload, z_sample_payload(sample)))
  {
    zxp_sample_drop(destination);
    return false;
  }
  return true;
}

static size_t zxp_bytes_heap_size(const zxp_bytes_t *bytes)
{
  return bytes->data == NULL ? 0 : term_binary_heap_size(bytes->size);
}

size_t zxp_sample_heap_size(const zxp_sample_t *sample)
{
  return term_map_size_in_terms(10) + zxp_bytes_heap_size(&sample->attachment) +
         zxp_bytes_heap_size(&sample->encoding) + zxp_bytes_heap_size(&sample->keyexpr) +
         zxp_bytes_heap_size(&sample->payload) +
         (sample->has_timestamp ? term_binary_heap_size(63) : 0);
}

static term zxp_bytes_to_term(Context *ctx, const zxp_bytes_t *bytes)
{
  return bytes->data == NULL ? term_nil()
                             : zxp_avm_binary_from_bytes(ctx, bytes->data, bytes->size);
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

term zxp_struct_from_zp_sample(Context *ctx, const zxp_sample_t *sample)
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
      zxp_bytes_to_term(ctx, &sample->attachment),
      zxp_atom_from_zp_congestion_control(sample->congestion_control),
      zxp_bytes_to_term(ctx, &sample->encoding),
      zxp_atom_from_zp_express(sample->express),
      zxp_bytes_to_term(ctx, &sample->keyexpr),
      zxp_atom_from_zp_sample_kind(sample->kind),
      zxp_bytes_to_term(ctx, &sample->payload),
      zxp_atom_from_zp_priority(sample->priority),
      zxp_binary_from_zp_timestamp(ctx, sample->has_timestamp ? &sample->timestamp : NULL),
  };

  return zxp_avm_map_from_arrays(ctx, keys, values, 10);
}
