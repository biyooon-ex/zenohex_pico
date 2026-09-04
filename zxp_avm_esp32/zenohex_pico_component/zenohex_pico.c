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

#include "config.h"
#include "session.h"

static void zenohex_pico_init_nif(GlobalContext *global){
  ErlNifEnv env;
  erl_nif_env_partial_init_from_globalcontext(&env, global);
  zxp_config_enif_init_resource_type(&env);
  zxp_session_enif_init_resource_type(&env);
}

// NIF 関数を保持する構造体
static const struct Nif config_default_nif = {
  .base.type = NIFFunctionType,
  .nif_ptr   = zxp_config_default // NIF 関数実体のポインタ
};

static const struct Nif config_get_nif = {
  .base.type = NIFFunctionType,
  .nif_ptr   = zxp_config_get
};

static const struct Nif config_insert_nif = {
  .base.type = NIFFunctionType,
  .nif_ptr   = zxp_config_insert
};

static const struct Nif session_open_nif = {
  .base.type = NIFFunctionType,
  .nif_ptr   = zxp_session_open // NIF 関数実体のポインタ
};

static const struct Nif session_close_nif = {
  .base.type = NIFFunctionType,
  .nif_ptr   = zxp_session_close
};

// NIF の呼び出しと関数実体を紐付ける関数
const struct Nif *zenohex_pico_get_nif(const char *nifname)
{
  if (strcmp("zenohex_pico_nif:config_default/0", nifname) == 0 ||
      strcmp("Elixir.ZenohexPico.Nif:config_default/0", nifname) == 0) {
    return &config_default_nif;
  }
  if (strcmp("zenohex_pico_nif:config_get/2", nifname) == 0 ||
      strcmp("Elixir.ZenohexPico.Nif:config_get/2", nifname) == 0) {
    return &config_get_nif;
  }
  if (strcmp("zenohex_pico_nif:config_insert/3", nifname) == 0 ||
      strcmp("Elixir.ZenohexPico.Nif:config_insert/3", nifname) == 0) {
    return &config_insert_nif;
  }
  if (strcmp("zenohex_pico_nif:session_open/1", nifname) == 0 ||
      strcmp("Elixir.ZenohexPico.Nif:session_open/1", nifname) == 0) {
    return &session_open_nif;
  }
  if (strcmp("zenohex_pico_nif:session_close/1", nifname) == 0 ||
      strcmp("Elixir.ZenohexPico.Nif:session_close/1", nifname) == 0) {
    return &session_close_nif;
  }
  return NULL;
}

// NIF を登録するマクロ
REGISTER_NIF_COLLECTION(zenohex_pico, zenohex_pico_init_nif, NULL, zenohex_pico_get_nif)
