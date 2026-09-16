#include <nifs.h>
#include <stddef.h>
#include <zenoh-pico.h>

extern bool zxp_sample_heap_size(const z_loaned_sample_t *sample, size_t *heap_size);
extern term zxp_struct_from_zp_sample(Context *ctx, const z_loaned_sample_t *sample);
