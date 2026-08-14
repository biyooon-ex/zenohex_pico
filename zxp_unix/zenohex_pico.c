#include <erl_nif.h>
#include <zenoh-pico.h>

#include "config.h"
#include "macro.h"
#include "session.h"
#include "term.h"

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM info)
{
  UNUSED(priv);
  UNUSED(info);

  zxp_init_atom(env);
  zxp_config_enif_init_resource_type(env);
  zxp_session_enif_init_resource_type(env);
  return 0;
}

static ErlNifFunc nif_funcs[] = {
    {"session_open", 1, zxp_session_open, 0},
    {"session_close", 1, zxp_session_close, 0},
    {"config_default", 0, zxp_config_default, 0},
    {"config_get", 2, zxp_config_get, 0},
    {"config_insert", 3, zxp_config_insert, 0},
    {"test_raise", 0, zxp_test_raise, 0},
};

ERL_NIF_INIT(Elixir.ZenohexPico.Nif, nif_funcs, load, NULL, NULL, NULL)
