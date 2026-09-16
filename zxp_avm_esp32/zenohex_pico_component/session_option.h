#include <nifs.h>
#include <stdbool.h>
#include <zenoh-pico.h>

// session_put_option

typedef struct
{
  z_put_options_t options;
  z_owned_encoding_t encoding;
  z_owned_bytes_t attachment;
  z_timestamp_t timestamp;
} zxp_session_put_options_t;

extern void zxp_session_put_options_drop(zxp_session_put_options_t *put_options);
extern bool zxp_session_put_options_new(Context *ctx, zxp_session_put_options_t **put_options,
                                        term *error);
extern bool zxp_session_put_options_init(Context *ctx, term options,
                                         zxp_session_put_options_t *put_options, term *error);
extern z_put_options_t *zxp_session_put_options_loan(zxp_session_put_options_t *put_options);

// session_get_option

typedef struct
{
  z_get_options_t options;
  z_owned_bytes_t payload;
  z_owned_encoding_t encoding;
  z_owned_bytes_t attachment;
} zxp_session_get_options_t;

extern void zxp_session_get_options_drop(zxp_session_get_options_t *get_options);
extern bool zxp_session_get_options_new(Context *ctx, zxp_session_get_options_t **get_options,
                                        term *error);
extern bool zxp_session_get_options_init(Context *ctx, term options,
                                         zxp_session_get_options_t *get_options, term *error);
extern z_get_options_t *zxp_session_get_options_loan(zxp_session_get_options_t *get_options);
