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

static ErlNifResourceType *config_resource_type;

static void config_dtor(ErlNifEnv *env, void *obj)
{
  ZenohexPicoConfigResource *res = (ZenohexPicoConfigResource *) obj;
  z_drop(z_move(res->config));
}

static const ErlNifResourceTypeInit ZenohexPicoConfigResourceTypeInit = {
  .dtor = config_dtor,
  .members = 1
};

static void zenohex_pico_init_nif(GlobalContext *global){
  ErlNifEnv env;
  erl_nif_env_partial_init_from_globalcontext(&env, global);
  config_resource_type = enif_init_resource_type(&env, "zenohex_pico_config", &ZenohexPicoConfigResourceTypeInit, ERL_NIF_RT_CREATE, NULL);
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
  return OK_ATOM;
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
