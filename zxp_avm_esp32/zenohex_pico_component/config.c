#include <defaultatoms.h>
#include <erl_nif_priv.h>
#include <memory.h>
#include <nifs.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <term.h>
#include <utils.h>
#include <zenoh-pico.h>

#include "avm_compat.h"
#include "config.h"

ErlNifResourceType *zxp_config_resource_type = NULL;

static void zxp_config_dtor(ErlNifEnv *env, void *obj)
{
  UNUSED(env);

  z_owned_config_t *config = (z_owned_config_t *)obj;
  z_drop(z_move(*config));
}

static const ErlNifResourceTypeInit ZxpConfigResourceTypeInit = {
    .dtor = zxp_config_dtor,
    .members = 1,
};

bool zxp_config_enif_init_resource_type(ErlNifEnv *env)
{
  zxp_config_resource_type = enif_init_resource_type(
      env, "zxp_config", &ZxpConfigResourceTypeInit, ERL_NIF_RT_CREATE, NULL);
  return zxp_config_resource_type != NULL;
}

term zxp_config_default(Context *ctx, int argc, term argv[])
{
  UNUSED(argc);
  UNUSED(argv);

  z_owned_config_t config;
  z_internal_null(&config);
  z_result_t ret = z_config_default(&config);

  if (ret != Z_OK)
  {
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  z_owned_config_t *config_p = enif_alloc_resource(zxp_config_resource_type, sizeof(*config_p));
  if (config_p == NULL)
  {
    z_drop(z_move(config));
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  z_internal_null(config_p);
  z_take(config_p, z_move(config));

  if (memory_ensure_free(ctx, TERM_BOXED_RESOURCE_SIZE + TUPLE_SIZE(2)) != MEMORY_GC_OK)
  {
    enif_release_resource(config_p);
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  term config_ref = term_from_resource(config_p, &ctx->heap);
  enif_release_resource(config_p);

  return zxp_make_tuple2(ctx, OK_ATOM, config_ref);
}

term zxp_config_get(Context *ctx, int argc, term argv[])
{
  UNUSED(argc);

  ErlNifEnv *env = erl_nif_env_from_context(ctx);
  z_owned_config_t *config_p = NULL;
  if (!enif_get_resource(env, argv[0], zxp_config_resource_type, (void **)&config_p))
  {
    RAISE_ERROR(BADARG_ATOM);
  }

  if (!term_is_integer(argv[1]))
  {
    RAISE_ERROR(BADARG_ATOM);
  }

  avm_int_t key = term_to_int(argv[1]);
  if (key < 0 || key > UINT8_MAX)
  {
    RAISE_ERROR(BADARG_ATOM);
  }

  const char *value = zp_config_get(z_loan(*config_p), (uint8_t)key);
  if (value == NULL)
  {
    if (memory_ensure_free(ctx, TUPLE_SIZE(2)) != MEMORY_GC_OK)
    {
      RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    }
    return zxp_make_tuple2(ctx, ERROR_ATOM, not_found_atom);
  }

  size_t len = strlen(value);
  char *value_copy = malloc(len);
  if (len != 0 && value_copy == NULL)
  {
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }
  memcpy(value_copy, value, len);

  if (memory_ensure_free(ctx, term_binary_heap_size(len) + TUPLE_SIZE(2)) != MEMORY_GC_OK)
  {
    free(value_copy);
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  term binary = zxp_binary_from_bytes(ctx, value_copy, len);
  free(value_copy);

  return zxp_make_tuple2(ctx, OK_ATOM, binary);
}

term zxp_config_insert(Context *ctx, int argc, term argv[])
{
  UNUSED(argc);

  ErlNifEnv *env = erl_nif_env_from_context(ctx);
  z_owned_config_t *config_p = NULL;
  if (!enif_get_resource(env, argv[0], zxp_config_resource_type, (void **)&config_p))
  {
    RAISE_ERROR(BADARG_ATOM);
  }

  if (!term_is_integer(argv[1]))
  {
    RAISE_ERROR(BADARG_ATOM);
  }

  if (!term_is_binary(argv[2]))
  {
    RAISE_ERROR(BADARG_ATOM);
  }

  avm_int_t key = term_to_int(argv[1]);
  if (key < 0 || key > UINT8_MAX)
  {
    RAISE_ERROR(BADARG_ATOM);
  }

  size_t len = term_binary_size(argv[2]);
  const char *data = term_binary_data(argv[2]);
  if (memchr(data, '\0', len) != NULL)
  {
    RAISE_ERROR(BADARG_ATOM);
  }

  char *value = malloc(len + 1);
  if (value == NULL)
  {
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  memcpy(value, data, len);
  value[len] = '\0';

  z_owned_config_t *new_config_p =
      enif_alloc_resource(zxp_config_resource_type, sizeof(*new_config_p));
  if (new_config_p == NULL)
  {
    free(value);
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  z_internal_null(new_config_p);

  {
    z_result_t ret = z_config_clone(new_config_p, z_loan(*config_p));
    if (ret != Z_OK)
    {
      free(value);
      enif_release_resource(new_config_p);
      RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    }
  }

  {
    z_result_t ret = zp_config_insert(z_loan_mut(*new_config_p), (uint8_t)key, value);
    free(value);
    if (ret != Z_OK)
    {
      enif_release_resource(new_config_p);
      RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    }
  }

  if (memory_ensure_free(ctx, TERM_BOXED_RESOURCE_SIZE + TUPLE_SIZE(2)) != MEMORY_GC_OK)
  {
    enif_release_resource(new_config_p);
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  term new_config_ref = term_from_resource(new_config_p, &ctx->heap);
  enif_release_resource(new_config_p);

  return zxp_make_tuple2(ctx, OK_ATOM, new_config_ref);
}
