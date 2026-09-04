#include <defaultatoms.h>
#include <erl_nif_priv.h>
#include <memory.h>
#include <nifs.h>
#include <term.h>
#include <zenoh-pico.h>

#include "config.h"

ErlNifResourceType *zxp_config_resource_type = NULL;
static term not_found_atom;

typedef struct {
  z_owned_config_t config;
} zxp_config_resource_t;

static void zxp_config_dtor(ErlNifEnv *env, void *obj)
{
  (void) env;

  zxp_config_resource_t *resource = obj;
  z_drop(z_move(resource->config));
}

static const ErlNifResourceTypeInit ZxpConfigResourceTypeInit = {
  .dtor = zxp_config_dtor,
  .members = 1,
};

bool zxp_config_enif_init_resource_type(ErlNifEnv *env)
{
  zxp_config_resource_type = enif_init_resource_type(
      env, "zxp_config", &ZxpConfigResourceTypeInit, ERL_NIF_RT_CREATE, NULL);
  not_found_atom = globalcontext_make_atom(env->global, ATOM_STR("\x9", "not_found"));
  return zxp_config_resource_type != NULL;
}

term zxp_config_default(Context *ctx, int argc, term argv[])
{
  (void) argc;
  (void) argv;

  z_owned_config_t config;
  z_internal_null(&config);
  if (z_config_default(&config) != Z_OK) {
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  zxp_config_resource_t *resource =
      enif_alloc_resource(zxp_config_resource_type, sizeof(*resource));
  if (resource == NULL) {
    z_drop(z_move(config));
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  z_internal_null(&resource->config);
  z_take(&resource->config, z_move(config));

  if (memory_ensure_free(ctx, TERM_BOXED_RESOURCE_SIZE + TUPLE_SIZE(2)) != MEMORY_GC_OK) {
    enif_release_resource(resource);
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  term config_ref = term_from_resource(resource, &ctx->heap);
  enif_release_resource(resource);

  term result = term_alloc_tuple(2, &ctx->heap);
  term_put_tuple_element(result, 0, OK_ATOM);
  term_put_tuple_element(result, 1, config_ref);
  return result;
}

term zxp_config_get(Context *ctx, int argc, term argv[])
{
  (void) argc;

  ErlNifEnv *env = erl_nif_env_from_context(ctx);
  zxp_config_resource_t *resource = NULL;
  if (!enif_get_resource(env, argv[0], zxp_config_resource_type, (void **) &resource) ||
      !term_is_integer(argv[1])) {
    RAISE_ERROR(BADARG_ATOM);
  }

  avm_int_t key = term_to_int(argv[1]);
  if (key < 0 || key > UINT8_MAX) {
    RAISE_ERROR(BADARG_ATOM);
  }

  const char *value = zp_config_get(z_loan(resource->config), (uint8_t) key);
  if (value == NULL) {
    if (memory_ensure_free_with_roots(ctx, TUPLE_SIZE(2), argc, argv, MEMORY_CAN_SHRINK) !=
        MEMORY_GC_OK) {
      RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    }
    term result = term_alloc_tuple(2, &ctx->heap);
    term_put_tuple_element(result, 0, ERROR_ATOM);
    term_put_tuple_element(result, 1, not_found_atom);
    return result;
  }

  size_t value_size = strlen(value);
  if (memory_ensure_free_with_roots(
          ctx, term_binary_heap_size(value_size) + TUPLE_SIZE(2), argc, argv, MEMORY_CAN_SHRINK) !=
      MEMORY_GC_OK) {
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  term binary = term_from_literal_binary(value, value_size, &ctx->heap, ctx->global);
  term result = term_alloc_tuple(2, &ctx->heap);
  term_put_tuple_element(result, 0, OK_ATOM);
  term_put_tuple_element(result, 1, binary);
  return result;
}

term zxp_config_insert(Context *ctx, int argc, term argv[])
{
  (void) argc;

  ErlNifEnv *env = erl_nif_env_from_context(ctx);
  zxp_config_resource_t *resource = NULL;
  if (!enif_get_resource(env, argv[0], zxp_config_resource_type, (void **) &resource) ||
      !term_is_integer(argv[1]) || !term_is_binary(argv[2])) {
    RAISE_ERROR(BADARG_ATOM);
  }

  avm_int_t key = term_to_int(argv[1]);
  if (key < 0 || key > UINT8_MAX) {
    RAISE_ERROR(BADARG_ATOM);
  }

  size_t value_size = term_binary_size(argv[2]);
  const char *binary_data = term_binary_data(argv[2]);
  if (memchr(binary_data, '\0', value_size) != NULL) {
    RAISE_ERROR(BADARG_ATOM);
  }

  char *value = malloc(value_size + 1);
  if (value == NULL) {
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }
  memcpy(value, binary_data, value_size);
  value[value_size] = '\0';

  zxp_config_resource_t *new_resource =
      enif_alloc_resource(zxp_config_resource_type, sizeof(*new_resource));
  if (new_resource == NULL) {
    free(value);
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }
  z_internal_null(&new_resource->config);

  z_result_t result = z_config_clone(&new_resource->config, z_loan(resource->config));
  if (result == Z_OK) {
    result = zp_config_insert(z_loan_mut(new_resource->config), (uint8_t) key, value);
  }
  free(value);
  if (result != Z_OK) {
    enif_release_resource(new_resource);
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  if (memory_ensure_free_with_roots(ctx, TERM_BOXED_RESOURCE_SIZE + TUPLE_SIZE(2), argc, argv,
          MEMORY_CAN_SHRINK) != MEMORY_GC_OK) {
    enif_release_resource(new_resource);
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  term config_ref = term_from_resource(new_resource, &ctx->heap);
  enif_release_resource(new_resource);
  term response = term_alloc_tuple(2, &ctx->heap);
  term_put_tuple_element(response, 0, OK_ATOM);
  term_put_tuple_element(response, 1, config_ref);
  return response;
}
