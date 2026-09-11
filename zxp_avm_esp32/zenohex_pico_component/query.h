#ifndef ZXP_QUERY_H
#define ZXP_QUERY_H

#include <nifs.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
  uint8_t *data;
  size_t size;
} zxp_bytes_t;

typedef struct
{
  zxp_bytes_t payload;
  zxp_bytes_t encoding;
} zxp_reply_error_t;

extern void zxp_reply_error_drop(zxp_reply_error_t *reply_error);
extern bool zxp_reply_error_from_zp_reply_err(zxp_reply_error_t *destination,
                                              const z_loaned_reply_err_t *reply_err);
extern size_t zxp_reply_error_heap_size(const zxp_reply_error_t *reply_error);
extern term zxp_struct_from_zp_reply_err(Context *ctx, const zxp_reply_error_t *reply_err);

#endif
