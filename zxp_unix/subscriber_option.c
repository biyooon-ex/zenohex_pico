#include <erl_nif.h>
#include <stdbool.h>
#include <zenoh-pico.h>

#include "subscriber_option.h"
#include "zxp_term.h"

bool zxp_subscriber_options_init(ErlNifEnv *env, ERL_NIF_TERM term, z_subscriber_options_t *options)
{
  // For embedded deployments, we determined that messages published by the same
  // Zenoh-Pico session on a device do not need to be received by subscribers on
  // that device. We do not use a Zenoh-Pico library with
  // Z_FEATURE_LOCAL_SUBSCRIBER enabled, so allowed_origin is not supported.
  (void)options;
  return enif_is_empty_list(env, term);
}
