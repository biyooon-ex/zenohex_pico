#include <nifs.h>
#include <stdbool.h>
#include <stddef.h>
#include <zenoh-pico.h>

extern bool zxp_timestamp_from_binary(const char *data, size_t size, z_timestamp_t *timestamp);
extern term zxp_binary_from_zp_timestamp(Context *ctx, const z_timestamp_t *timestamp);
