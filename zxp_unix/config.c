#include <erl_nif.h>
#include <stdbool.h>
#include <zenoh-pico.h>

#include "macro.h"
#include "term.h"

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

ERL_NIF_TERM zxp_config_default(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  UNUSED(argc);
  UNUSED(argv);

  z_owned_config_t config;
  z_result_t ret = z_config_default(&config);

  if (ret != Z_OK)
  {
    return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
  }

  z_owned_config_t *config_p = enif_alloc_resource(zxp_config_resource_type, sizeof(*config_p));
  if (config_p == NULL)
  {
    z_drop(z_move(config));
    return zxp_raise_null_pointer(env, __FILE__, __LINE__);
  }

  z_internal_null(config_p);
  z_take(config_p, z_move(config));

  ERL_NIF_TERM config_ref = enif_make_resource(env, config_p);
  enif_release_resource(config_p);

  return enif_make_tuple2(env, ok_atom, config_ref);
}

ERL_NIF_TERM zxp_config_get(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  UNUSED(argc);

  z_owned_config_t *config_p = NULL;
  if (!enif_get_resource(env, argv[0], zxp_config_resource_type, (void **)&config_p))
  {
    return enif_make_badarg(env);
  }

  unsigned int key = 0;
  if (!enif_get_uint(env, argv[1], (unsigned int *)&key))
  {
    return enif_make_badarg(env);
  }

  if (key > UINT8_MAX)
  {
    return enif_make_badarg(env);
  }

  const char *value = zp_config_get(z_loan(*config_p), (uint8_t)key);
  if (value == NULL)
  {
    return enif_make_tuple2(env, error_atom, not_found_atom);
  }

  size_t len = strlen(value);
  ErlNifBinary bin;
  if (!enif_alloc_binary(len, &bin))
  {
    return zxp_raise(env, __FILE__, __LINE__, "enif_alloc_binary returns false");
  }

  memcpy(bin.data, value, len);
  ERL_NIF_TERM binary = enif_make_binary(env, &bin);

  return enif_make_tuple2(env, ok_atom, binary);
}

ERL_NIF_TERM zxp_config_insert(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
  UNUSED(argc);

  z_owned_config_t *config_p = NULL;
  if (!enif_get_resource(env, argv[0], zxp_config_resource_type, (void **)&config_p))
  {
    return enif_make_badarg(env);
  }

  unsigned int key = 0;
  if (!enif_get_uint(env, argv[1], &key))
  {
    return enif_make_badarg(env);
  }

  if (key > UINT8_MAX)
  {
    return enif_make_badarg(env);
  }

  ErlNifBinary bin;
  if (!enif_inspect_binary(env, argv[2], &bin))
  {
    return enif_make_badarg(env);
  }

  char *value = enif_alloc(bin.size + 1);
  if (value == NULL)
  {
    return zxp_raise_null_pointer(env, __FILE__, __LINE__);
  }

  memcpy(value, bin.data, bin.size);
  value[bin.size] = '\0';

  z_owned_config_t *new_config_p =
      enif_alloc_resource(zxp_config_resource_type, sizeof(*new_config_p));
  if (new_config_p == NULL)
  {
    enif_free(value);
    return zxp_raise_null_pointer(env, __FILE__, __LINE__);
  }

  z_internal_null(new_config_p);

  {
    z_result_t ret = z_config_clone(new_config_p, z_loan(*config_p));
    if (ret != Z_OK)
    {
      enif_free(value);
      enif_release_resource(new_config_p);
      return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
    }
  }

  {
    z_result_t ret = zp_config_insert(z_loan_mut(*new_config_p), key, value);
    enif_free(value);
    if (ret != Z_OK)
    {
      enif_release_resource(new_config_p);
      return zxp_error_tuple_zp(env, __FILE__, __LINE__, ret);
    }
  }

  ERL_NIF_TERM new_config_ref = enif_make_resource(env, new_config_p);
  enif_release_resource(new_config_p);

  return enif_make_tuple2(env, ok_atom, new_config_ref);
}
