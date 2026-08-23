#include <erl_nif.h>
#include <stdbool.h>
#include <zenoh-pico.h>

extern bool zxp_subscriber_options_init(ErlNifEnv *env, ERL_NIF_TERM term,
                                        z_subscriber_options_t *options);
