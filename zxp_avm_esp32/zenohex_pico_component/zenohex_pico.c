// Defines OK_ATOM.
#include <defaultatoms.h>
#include <erl_nif_priv.h>
// Defines struct Nif.
#include <nifs.h>
// Defines REGISTER_NIF_COLLECTION.
#include <portnifloader.h>
// Defines strcmp.
#include <string.h>
// Defines term.
#include <term.h>

#include <zenoh-pico.h>

#include "config.h"
#include "session.h"
#include "session_option.h"
#include "zxp_term.h"

static void zenohex_pico_init_nif(GlobalContext *global)
{
  ErlNifEnv env;
  erl_nif_env_partial_init_from_globalcontext(&env, global);
  zxp_init_atom(global);
  zxp_config_enif_init_resource_type(&env);
  zxp_session_enif_init_resource_type(&env);
}

// NIF function descriptors.
static const struct Nif config_default_nif = {
    .base.type = NIFFunctionType,
    .nif_ptr = zxp_config_default,
};

static const struct Nif config_get_nif = {
    .base.type = NIFFunctionType,
    .nif_ptr = zxp_config_get,
};

static const struct Nif config_insert_nif = {
    .base.type = NIFFunctionType,
    .nif_ptr = zxp_config_insert,
};

static const struct Nif session_open_nif = {
    .base.type = NIFFunctionType,
    .nif_ptr = zxp_session_open,
};

static const struct Nif session_close_nif = {
    .base.type = NIFFunctionType,
    .nif_ptr = zxp_session_close,
};

static const struct Nif session_put_nif = {
    .base.type = NIFFunctionType,
    .nif_ptr = zxp_session_put,
};

static const struct Nif session_get_nif = {
    .base.type = NIFFunctionType,
    .nif_ptr = zxp_session_get,
};

static const struct Nif test_raise_nif = {
    .base.type = NIFFunctionType,
    .nif_ptr = zxp_test_raise,
};

// Resolves NIF names to their function descriptors.
const struct Nif *zenohex_pico_get_nif(const char *nifname)
{
  if (strcmp("zenohex_pico_nif:config_default/0", nifname) == 0 ||
      strcmp("Elixir.ZenohexPico.Nif:config_default/0", nifname) == 0)
  {
    return &config_default_nif;
  }
  if (strcmp("zenohex_pico_nif:config_get/2", nifname) == 0 ||
      strcmp("Elixir.ZenohexPico.Nif:config_get/2", nifname) == 0)
  {
    return &config_get_nif;
  }
  if (strcmp("zenohex_pico_nif:config_insert/3", nifname) == 0 ||
      strcmp("Elixir.ZenohexPico.Nif:config_insert/3", nifname) == 0)
  {
    return &config_insert_nif;
  }
  if (strcmp("zenohex_pico_nif:session_open/1", nifname) == 0 ||
      strcmp("Elixir.ZenohexPico.Nif:session_open/1", nifname) == 0)
  {
    return &session_open_nif;
  }
  if (strcmp("zenohex_pico_nif:session_close/1", nifname) == 0 ||
      strcmp("Elixir.ZenohexPico.Nif:session_close/1", nifname) == 0)
  {
    return &session_close_nif;
  }
  if (strcmp("zenohex_pico_nif:session_put/4", nifname) == 0 ||
      strcmp("Elixir.ZenohexPico.Nif:session_put/4", nifname) == 0)
  {
    return &session_put_nif;
  }
  if (strcmp("zenohex_pico_nif:session_get/4", nifname) == 0 ||
      strcmp("Elixir.ZenohexPico.Nif:session_get/4", nifname) == 0)
  {
    return &session_get_nif;
  }
  if (strcmp("zenohex_pico_nif:test_raise/0", nifname) == 0 ||
      strcmp("Elixir.ZenohexPico.Nif:test_raise/0", nifname) == 0)
  {
    return &test_raise_nif;
  }
  return NULL;
}

// Registers the AtomVM NIF collection.
REGISTER_NIF_COLLECTION(zenohex_pico, zenohex_pico_init_nif, NULL, zenohex_pico_get_nif)
