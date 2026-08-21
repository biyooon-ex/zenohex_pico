#include <erl_nif.h>
#include <zenoh-pico.h>

extern void zxp_session_put_options_drop(z_put_options_t *options);
extern bool zxp_session_put_options(ErlNifEnv *env, ERL_NIF_TERM term, z_put_options_t *options,
                                    z_owned_encoding_t *encoding, z_owned_bytes_t *attachment);
