// OK_ATOM が定義されているヘッダ
#include <defaultatoms.h>
// Nif 構造体が定義されているヘッダ
#include <nifs.h>
// REGISTER_NIF_COLLECTION が定義されているヘッダ
#include <portnifloader.h>
// strcmp が定義されているヘッダ
#include <string.h>
// term 型が定義されているヘッダ
#include <term.h>

#include <zenoh-pico.h>

static term config_default(Context *ctx, int argc, term argv[])
{
  z_owned_config_t config;
  z_result_t ret = z_config_default(&config);

  if(ret != Z_OK) {
    return ERROR_ATOM;
  }
  return OK_ATOM;
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
REGISTER_NIF_COLLECTION(zenohex_pico, NULL, NULL, zenohex_pico_get_nif)
