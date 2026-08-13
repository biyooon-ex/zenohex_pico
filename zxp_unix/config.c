#include <erl_nif.h>
#include <zenoh-pico.h>

ErlNifResourceType *config_resource_type = NULL;

static void config_dtor(ErlNifEnv *env, void *obj)
{
  z_owned_config_t *config = (z_owned_config_t *)obj;
  z_drop(z_move(*config));
}

static const ErlNifResourceTypeInit ZenohexPicoConfigResourceTypeInit = {.dtor = config_dtor,
                                                                         .members = 1};

void config_enif_init_resource_type(ErlNifEnv *env)
{
  config_resource_type = enif_init_resource_type(
      env, "zenohex_pico_config", &ZenohexPicoConfigResourceTypeInit, ERL_NIF_RT_CREATE, NULL);
}

ERL_NIF_TERM config_default(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  z_owned_config_t config;
  z_result_t ret = z_config_default(&config);

  if (ret != Z_OK)
  {
  }

  z_owned_config_t *config_p = enif_alloc_resource(config_resource_type, sizeof(z_owned_config_t));
  if (config_p == NULL)
  {
  }

  z_internal_null(config_p);
  z_take(config_p, z_move(config));

  ERL_NIF_TERM config_ref = enif_make_resource(env, config_p);
  enif_release_resource(config_p);

  return enif_make_tuple2(env, enif_make_atom(env, "ok"), config_ref);
}
