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

  init_atom(env);
  config_enif_init_resource_type(env);
  session_enif_init_resource_type(env);
  return 0;
}

static ErlNifFunc nif_funcs[] = {
    {"session_open", 1, session_open, 0},
    {"config_default", 0, config_default, 0},
};

ERL_NIF_INIT(Elixir.ZenohexPico.Nif, nif_funcs, load, NULL, NULL, NULL)
