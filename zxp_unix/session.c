#include <erl_nif.h>
#include <zenoh-pico.h>

#include "config.h"
#include "term.h"

ErlNifResourceType *session_resource_type = NULL;

static void session_dtor(ErlNifEnv *env, void *obj)
{
  z_owned_session_t *session_p = (z_owned_session_t *)obj;
  z_drop(z_move(*session_p));
}

static const ErlNifResourceTypeInit ZenohexPicoSessionResourceTypeInit = {
    .dtor = session_dtor,
    .members = 1,
};

void session_enif_init_resource_type(ErlNifEnv *env)
{
  session_resource_type = enif_init_resource_type(
      env, "zenohex_pico_config", &ZenohexPicoSessionResourceTypeInit, ERL_NIF_RT_CREATE, NULL);
}

ERL_NIF_TERM session_open(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  z_owned_config_t *config_p = NULL;
  if (!enif_get_resource(env, argv[0], config_resource_type, (void **)&config_p))
  {
    return enif_make_badarg(env);
  }

  z_owned_config_t config;
  {
    z_result_t ret = z_clone(&config, z_loan(*config_p));

    if (ret != Z_OK)
    {
      return error_tuple_zp(env, __FILE__, __LINE__, ret);
    }
  }

  z_owned_session_t session;
  {
    z_result_t ret = z_open(&session, z_move(config), NULL);

    if (ret != Z_OK)
    {
      return error_tuple_zp(env, __FILE__, __LINE__, ret);
    }
  }

  z_owned_session_t *session_p = enif_alloc_resource(session_resource_type, sizeof(session_p));
  if (session_p == NULL)
  {
    return raise_null_pointer(env, __FILE__, __LINE__);
  }

  z_internal_null(session_p);
  z_take(session_p, z_move(session));

  ERL_NIF_TERM session_ref = enif_make_resource(env, session_p);
  enif_release_resource(session_p);

  return enif_make_tuple2(env, ok_atom, session_ref);
}
