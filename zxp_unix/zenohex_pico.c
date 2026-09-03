#include <erl_nif.h>
#include <zenoh-pico.h>

#include "config.h"
#include "macro.h"
#include "session.h"
#include "subscriber.h"
#include "term.h"

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM info)
{
  UNUSED(priv);
  UNUSED(info);

  zxp_init_atom(env);
  if (!zxp_config_enif_init_resource_type(env) || !zxp_session_enif_init_resource_type(env) ||
      !zxp_subscriber_enif_init_resource_type(env))
  {
    return 1;
  }
  return 0;
}

static ErlNifFunc nif_funcs[] = {
    {"session_open", 1, zxp_session_open, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"session_close", 1, zxp_session_close, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"session_declare_subscriber", 4, zxp_session_declare_subscriber, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"session_put", 4, zxp_session_put, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"session_get", 4, zxp_session_get, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"subscriber_undeclare", 1, zxp_subscriber_undeclare, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"config_default", 0, zxp_config_default, 0},
    {"config_get", 2, zxp_config_get, 0},
    {"config_insert", 3, zxp_config_insert, 0},
    {"test_raise", 0, zxp_test_raise, 0},
};

ERL_NIF_INIT(Elixir.ZenohexPico.Nif, nif_funcs, load, NULL, NULL, NULL)
