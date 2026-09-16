#include <erl_nif.h>
#include <nifs.h>
#include <stdbool.h>

extern ErlNifResourceType *zxp_session_resource_type;

extern bool zxp_session_enif_init_resource_type(ErlNifEnv *env);
extern term zxp_session_open(Context *ctx, int argc, term argv[]);
extern term zxp_session_close(Context *ctx, int argc, term argv[]);
extern term zxp_session_put(Context *ctx, int argc, term argv[]);
extern term zxp_session_get(Context *ctx, int argc, term argv[]);
