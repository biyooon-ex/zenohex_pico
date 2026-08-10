// OK_ATOM が定義されているヘッダ
#include <defaultatoms.h>
//
#include <erl_nif_priv.h>
// Nif 構造体が定義されているヘッダ
#include <nifs.h>
// REGISTER_NIF_COLLECTION が定義されているヘッダ
#include <portnifloader.h>
// strcmp が定義されているヘッダ
#include <string.h>
// term 型が定義されているヘッダ
#include <term.h>

#include <zenoh-pico.h>

typedef struct {
  z_owned_config_t config;
} ZenohexPicoConfigResource;

typedef struct {
  z_owned_session_t session;
} ZenohexPicoSessionResource;

static ErlNifResourceType *config_resource_type;
static ErlNifResourceType *session_resource_type;

static void config_dtor(ErlNifEnv *env, void *obj)
{
  ZenohexPicoConfigResource *res = (ZenohexPicoConfigResource *) obj;
  z_drop(z_move(res->config));
}

static void session_dtor(ErlNifEnv *env, void *obj)
{
  ZenohexPicoSessionResource *res = (ZenohexPicoSessionResource *) obj;
  z_drop(z_move(res->session));
}

static const ErlNifResourceTypeInit ZenohexPicoConfigResourceTypeInit = {
  .dtor = config_dtor,
  .members = 1
};

static const ErlNifResourceTypeInit ZenohexPicoSessionResourceTypeInit = {
  .dtor = session_dtor,
  .members = 1
};

static void zenohex_pico_init_nif(GlobalContext *global){
  ErlNifEnv env;
  erl_nif_env_partial_init_from_globalcontext(&env, global);
  config_resource_type = enif_init_resource_type(&env, "zenohex_pico_config", &ZenohexPicoConfigResourceTypeInit, ERL_NIF_RT_CREATE, NULL);
  session_resource_type = enif_init_resource_type(&env, "zenohex_pico_session", &ZenohexPicoSessionResourceTypeInit, ERL_NIF_RT_CREATE, NULL);
}

static term error_reason_tuple(Context *ctx, const char* str)
{
  term reason = term_from_literal_binary(str, strlen(str), &ctx->heap, ctx->global);
  term tuple = term_alloc_tuple(2, &ctx->heap);
  term_put_tuple_element(tuple, 0, ERROR_ATOM);
  term_put_tuple_element(tuple, 1, reason);
  return tuple;
}

static term config_default(Context *ctx, int argc, term argv[])
{
  z_owned_config_t config;
  z_result_t ret = z_config_default(&config);

  if(ret != Z_OK) {
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  ZenohexPicoConfigResource *res = enif_alloc_resource(config_resource_type, sizeof(ZenohexPicoConfigResource));
  if (res == NULL) {
    z_drop(z_move(config));
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  z_internal_null(&res->config);
  z_take(&res->config, z_move(config));

  term obj = term_from_resource(res, &ctx->heap);
  enif_release_resource(res);

  term result = term_alloc_tuple(2, &ctx->heap);
  term_put_tuple_element(result, 0, OK_ATOM);
  term_put_tuple_element(result, 1, obj);

  return result;
}

// NIF 関数実体
static term session_open(Context *ctx, int argc, term argv[])
{
  ErlNifEnv *env = erl_nif_env_from_context(ctx);
  void *objp = NULL;
  if(!enif_get_resource(env, argv[0], config_resource_type, &objp)){
    RAISE_ERROR(BADARG_ATOM);
  }

  ZenohexPicoConfigResource *c_res = (ZenohexPicoConfigResource *) objp;

  z_owned_config_t config;
  {
    z_result_t ret = z_clone(&config, z_loan(c_res->config));

    if(ret != Z_OK) {
      return error_reason_tuple(ctx, "z_clone");
    }
  }

  z_owned_session_t session;
  {
    z_result_t ret = z_open(&session, z_move(config), NULL);

    if(ret != Z_OK) {
      return error_reason_tuple(ctx, "z_open");
    }
  }

  ZenohexPicoSessionResource *s_res = enif_alloc_resource(session_resource_type, sizeof(ZenohexPicoSessionResource));
  if (s_res == NULL) {
    z_drop(z_move(session));
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  z_internal_null(&s_res->session);
  z_take(&s_res->session, z_move(session));

  term obj = term_from_resource(s_res, &ctx->heap);
  enif_release_resource(s_res);

  term result = term_alloc_tuple(2, &ctx->heap);
  term_put_tuple_element(result, 0, OK_ATOM);
  term_put_tuple_element(result, 1, obj);
  return result;
}

// NIF 関数を保持する構造体
static const struct Nif config_default_nif = {
  .base.type = NIFFunctionType,
  .nif_ptr   = config_default // NIF 関数実体のポインタ
};

static const struct Nif session_open_nif = {
  .base.type = NIFFunctionType,
  .nif_ptr   = session_open // NIF 関数実体のポインタ
};

// NIF の呼び出しと関数実体を紐付ける関数
const struct Nif *zenohex_pico_get_nif(const char *nifname)
{
  if (strcmp("zenohex_pico_nif:config_default/0", nifname) == 0 ||
      strcmp("Elixir.ZenohexPico.Nif:config_default/0", nifname) == 0) {
    return &config_default_nif;
  }
  if (strcmp("zenohex_pico_nif:session_open/1", nifname) == 0 ||
      strcmp("Elixir.ZenohexPico.Nif:session_open/1", nifname) == 0) {
    return &session_open_nif;
  }
  return NULL;
}

// NIF を登録するマクロ
REGISTER_NIF_COLLECTION(zenohex_pico, zenohex_pico_init_nif, NULL, zenohex_pico_get_nif)
