#ifndef ZXP_TIMESTAMP_H
#define ZXP_TIMESTAMP_H

#include <stdbool.h>
#include <stddef.h>
#include <zenoh-pico.h>

bool zxp_timestamp_from_binary(const char *data, size_t size, z_timestamp_t *timestamp);

#endif
