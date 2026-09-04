#ifndef ZXP_SESSION_OPTION_H
#define ZXP_SESSION_OPTION_H

#include <stdbool.h>
#include <nifs.h>
#include <zenoh-pico.h>

typedef struct {
  z_put_options_t options;
  z_owned_encoding_t encoding;
  z_owned_bytes_t attachment;
  z_timestamp_t timestamp;
} zxp_session_put_options_t;

typedef struct {
  z_get_options_t options;
  z_owned_bytes_t payload;
  z_owned_encoding_t encoding;
  z_owned_bytes_t attachment;
} zxp_session_get_options_t;

void zxp_session_option_init_atoms(GlobalContext *global);
bool zxp_session_put_options_init(term options, zxp_session_put_options_t *put_options);
void zxp_session_put_options_drop(zxp_session_put_options_t *put_options);
bool zxp_session_get_options_init(term options, zxp_session_get_options_t *get_options);
void zxp_session_get_options_drop(zxp_session_get_options_t *get_options);

#endif
