#ifndef ZXP_CONFIG_H
#define ZXP_CONFIG_H

#include <erl_nif.h>
#include <nifs.h>

extern ErlNifResourceType *zxp_config_resource_type;
extern bool zxp_config_enif_init_resource_type(ErlNifEnv *env);
extern term zxp_config_default(Context *ctx, int argc, term argv[]);
extern term zxp_config_get(Context *ctx, int argc, term argv[]);
extern term zxp_config_insert(Context *ctx, int argc, term argv[]);

#endif
