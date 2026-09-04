#ifndef ZXP_QUERY_H
#define ZXP_QUERY_H

#include <stddef.h>
#include <nifs.h>

#include "sample.h"

typedef struct {
  zxp_bytes_t payload;
  zxp_bytes_t encoding;
} zxp_reply_error_t;

void zxp_query_init_atoms(GlobalContext *global);
void zxp_reply_error_drop(zxp_reply_error_t *reply_error);
size_t zxp_reply_error_heap_size(const zxp_reply_error_t *reply_error);
term zxp_reply_error_to_term(Context *ctx, const zxp_reply_error_t *reply_error);

#endif
