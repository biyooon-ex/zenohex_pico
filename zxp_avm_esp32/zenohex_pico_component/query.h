#ifndef ZXP_QUERY_H
#define ZXP_QUERY_H

#include <nifs.h>
#include <stddef.h>
#include <zenoh-pico.h>

extern bool zxp_reply_error_heap_size(const z_loaned_reply_err_t *reply_error, size_t *heap_size);
extern term zxp_struct_from_zp_reply_err(Context *ctx, const z_loaned_reply_err_t *reply_err);

#endif
