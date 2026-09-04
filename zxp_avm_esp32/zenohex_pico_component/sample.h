#ifndef ZXP_SAMPLE_H
#define ZXP_SAMPLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <nifs.h>
#include <zenoh-pico.h>

typedef struct {
  uint8_t *data;
  size_t size;
} zxp_bytes_t;

typedef struct {
  zxp_bytes_t attachment;
  zxp_bytes_t encoding;
  zxp_bytes_t key_expr;
  zxp_bytes_t payload;
  z_congestion_control_t congestion_control;
  bool express;
  z_sample_kind_t kind;
  z_priority_t priority;
  bool has_timestamp;
  z_timestamp_t timestamp;
} zxp_sample_t;

void zxp_sample_init_atoms(GlobalContext *global);
void zxp_sample_drop(zxp_sample_t *sample);
size_t zxp_sample_heap_size(const zxp_sample_t *sample);
term zxp_sample_to_term(Context *ctx, const zxp_sample_t *sample);

#endif
