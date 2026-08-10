#include <erl_nif.h>

#include <zenoh-pico.h>

typedef struct {
  z_owned_config_t config;
} ZenohexPicoConfigResource;

typedef struct {
  z_owned_session_t session;
} ZenohexPicoSessionResource;

static ErlNifResourceType *config_resource_type = NULL;
static ErlNifResourceType *session_resource_type = NULL;

static void config_dtor(ErlNifEnv *env, void *obj)
{
  ZenohexPicoConfigResource *res = (ZenohexPicoConfigResource *) obj;
  z_drop(z_move(res->config));
}

static void session_dtor(ErlNifEnv *env, void *obj) {
  ZenohexPicoSessionResource *res = (ZenohexPicoSessionResource *)obj;
  z_drop(z_move(res->session));
}

static int load(ErlNifEnv *env, void **priv, ERL_NIF_TERM info)
{
  config_resource_type = enif_open_resource_type(env, NULL, "zenohex_pico_config", config_dtor, ERL_NIF_RT_CREATE, NULL);
  session_resource_type = enif_open_resource_type(env, NULL, "zenohex_pico_session", session_dtor, ERL_NIF_RT_CREATE, NULL);
  return 0;
}

static ERL_NIF_TERM error_reason_tuple(ErlNifEnv *env, const char* str)
{

  ErlNifBinary bin;
  enif_alloc_binary(strlen(str), &bin);
  memcpy(bin.data, str, strlen(str));

  return enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_binary(env, &bin));
}

static ERL_NIF_TERM config_default(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  z_owned_config_t config;
  z_result_t ret = z_config_default(&config);

  if(ret != Z_OK) {
  }

  ZenohexPicoConfigResource *res = enif_alloc_resource(config_resource_type, sizeof(ZenohexPicoConfigResource));
  if (res == NULL) {
  }

  z_internal_null(&res->config);
  z_take(&res->config, z_move(config));

  ERL_NIF_TERM resource_term = enif_make_resource(env, res);
  enif_release_resource(res);

  return enif_make_tuple2(env, enif_make_atom(env, "ok"), resource_term);
}

static ERL_NIF_TERM session_open(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    ZenohexPicoConfigResource *c_res = NULL;
  if(!enif_get_resource(env, argv[0], config_resource_type, (void **)&c_res)){

  }

  z_owned_config_t config;
   {
     z_result_t ret = z_clone(&config, z_loan(c_res->config));

     if(ret != Z_OK) {
       return error_reason_tuple(env, "z_clone");
     }
   }

   z_owned_session_t session;
   {
     z_result_t ret = z_open(&session, z_move(config), NULL);

     if(ret != Z_OK) {
       return error_reason_tuple(env, "z_open");
     }
   }

  ZenohexPicoSessionResource *s_res = enif_alloc_resource(session_resource_type, sizeof(ZenohexPicoSessionResource));
  if (s_res == NULL) {
  }

  z_internal_null(&s_res->session);
  z_take(&s_res->session, z_move(session));

  ERL_NIF_TERM resource_term = enif_make_resource(env, s_res);
  enif_release_resource(s_res);

  return enif_make_tuple2(env, enif_make_atom(env, "ok"), resource_term);
}

static ErlNifFunc nif_funcs[] = {
  {"session_open", 1, session_open, 0},
  {"config_default", 0, config_default, 0}
};

ERL_NIF_INIT(Elixir.ZenohexPico.Nif, nif_funcs, load, NULL, NULL, NULL)
