#ifndef ZXP_SESSION_H
#define ZXP_SESSION_H

#include <erl_nif.h>
#include <nifs.h>

extern ErlNifResourceType *zxp_session_resource_type;

bool zxp_session_enif_init_resource_type(ErlNifEnv *env);
term zxp_session_open(Context *ctx, int argc, term argv[]);
term zxp_session_close(Context *ctx, int argc, term argv[]);
term zxp_session_put(Context *ctx, int argc, term argv[]);

#endif
