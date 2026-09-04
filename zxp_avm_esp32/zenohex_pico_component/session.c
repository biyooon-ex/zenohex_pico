#include <defaultatoms.h>
#include <erl_nif_priv.h>
#include <memory.h>
#include <nifs.h>
#include <pthread.h>
#include <term.h>
#include <zenoh-pico.h>

#include "config.h"
#include "session.h"
#include "session_option.h"

ErlNifResourceType *zxp_session_resource_type = NULL;
static term session_closed_atom;

typedef struct {
  pthread_mutex_t mutex;
  z_owned_session_t session;
  bool is_mutex_initialized;
} zxp_session_resource_t;

static term zxp_session_error_tuple(Context *ctx, const char *reason)
{
  size_t reason_size = strlen(reason);
  if (memory_ensure_free(ctx, term_binary_heap_size(reason_size) + TUPLE_SIZE(2)) != MEMORY_GC_OK) {
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  term reason_term = term_from_literal_binary(reason, reason_size, &ctx->heap, ctx->global);
  term result = term_alloc_tuple(2, &ctx->heap);
  term_put_tuple_element(result, 0, ERROR_ATOM);
  term_put_tuple_element(result, 1, reason_term);
  return result;
}

static void zxp_session_dtor(ErlNifEnv *env, void *obj)
{
  (void) env;

  zxp_session_resource_t *resource = obj;
  if (!resource->is_mutex_initialized) {
    return;
  }

  z_owned_session_t session;
  z_internal_null(&session);
  pthread_mutex_lock(&resource->mutex);
  if (z_internal_session_check(&resource->session)) {
    z_take(&session, z_move(resource->session));
  }
  pthread_mutex_unlock(&resource->mutex);

  if (z_internal_session_check(&session)) {
    z_drop(z_move(session));
  }
  pthread_mutex_destroy(&resource->mutex);
}

static const ErlNifResourceTypeInit ZxpSessionResourceTypeInit = {
  .dtor = zxp_session_dtor,
  .members = 1,
};

bool zxp_session_enif_init_resource_type(ErlNifEnv *env)
{
  zxp_session_resource_type = enif_init_resource_type(
      env, "zxp_session", &ZxpSessionResourceTypeInit, ERL_NIF_RT_CREATE, NULL);
  session_closed_atom = globalcontext_make_atom(env->global, ATOM_STR("\xE", "session_closed"));
  return zxp_session_resource_type != NULL;
}

term zxp_session_open(Context *ctx, int argc, term argv[])
{
  (void) argc;

  ErlNifEnv *env = erl_nif_env_from_context(ctx);
  z_owned_config_t *config_resource = NULL;
  if (!enif_get_resource(env, argv[0], zxp_config_resource_type, (void **) &config_resource)) {
    RAISE_ERROR(BADARG_ATOM);
  }

  z_owned_config_t config;
  z_internal_null(&config);
  if (z_clone(&config, z_loan(*config_resource)) != Z_OK) {
    return zxp_session_error_tuple(ctx, "z_clone");
  }

  z_owned_session_t session;
  z_internal_null(&session);
  if (z_open(&session, z_move(config), NULL) != Z_OK) {
    return zxp_session_error_tuple(ctx, "z_open");
  }

  zxp_session_resource_t *resource =
      enif_alloc_resource(zxp_session_resource_type, sizeof(*resource));
  if (resource == NULL) {
    z_drop(z_move(session));
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }
  z_internal_null(&resource->session);
  resource->is_mutex_initialized = false;
  if (pthread_mutex_init(&resource->mutex, NULL) != 0) {
    z_drop(z_move(session));
    enif_release_resource(resource);
    return zxp_session_error_tuple(ctx, "pthread_mutex_init");
  }

  resource->is_mutex_initialized = true;
  z_take(&resource->session, z_move(session));
  if (memory_ensure_free_with_roots(ctx, TERM_BOXED_RESOURCE_SIZE + TUPLE_SIZE(2), argc, argv,
          MEMORY_CAN_SHRINK) != MEMORY_GC_OK) {
    enif_release_resource(resource);
    RAISE_ERROR(OUT_OF_MEMORY_ATOM);
  }

  term session_ref = term_from_resource(resource, &ctx->heap);
  enif_release_resource(resource);
  term result = term_alloc_tuple(2, &ctx->heap);
  term_put_tuple_element(result, 0, OK_ATOM);
  term_put_tuple_element(result, 1, session_ref);
  return result;
}

term zxp_session_close(Context *ctx, int argc, term argv[])
{
  (void) argc;

  ErlNifEnv *env = erl_nif_env_from_context(ctx);
  zxp_session_resource_t *resource = NULL;
  if (!enif_get_resource(env, argv[0], zxp_session_resource_type, (void **) &resource)) {
    RAISE_ERROR(BADARG_ATOM);
  }

  z_owned_session_t session;
  z_internal_null(&session);
  pthread_mutex_lock(&resource->mutex);
  if (!z_internal_session_check(&resource->session)) {
    pthread_mutex_unlock(&resource->mutex);
    if (memory_ensure_free_with_roots(ctx, TUPLE_SIZE(2), argc, argv, MEMORY_CAN_SHRINK) !=
        MEMORY_GC_OK) {
      RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    }
    term result = term_alloc_tuple(2, &ctx->heap);
    term_put_tuple_element(result, 0, ERROR_ATOM);
    term_put_tuple_element(result, 1, session_closed_atom);
    return result;
  }
  z_take(&session, z_move(resource->session));
  pthread_mutex_unlock(&resource->mutex);

  z_drop(z_move(session));
  return OK_ATOM;
}

term zxp_session_put(Context *ctx, int argc, term argv[])
{
  (void) argc;

  ErlNifEnv *env = erl_nif_env_from_context(ctx);
  zxp_session_resource_t *resource = NULL;
  if (!enif_get_resource(env, argv[0], zxp_session_resource_type, (void **) &resource) ||
      !term_is_binary(argv[1]) || !term_is_binary(argv[2])) {
    RAISE_ERROR(BADARG_ATOM);
  }

  zxp_session_put_options_t put_options;
  if (!zxp_session_put_options_init(argv[3], &put_options)) {
    RAISE_ERROR(BADARG_ATOM);
  }

  z_owned_keyexpr_t keyexpr;
  z_internal_null(&keyexpr);
  z_result_t result = z_keyexpr_from_substr(
      &keyexpr, term_binary_data(argv[1]), term_binary_size(argv[1]));
  if (result != Z_OK) {
    zxp_session_put_options_drop(&put_options);
    return zxp_session_error_tuple(ctx, "z_keyexpr_from_substr");
  }

  z_owned_bytes_t payload;
  z_internal_null(&payload);
  result = z_bytes_copy_from_buf(
      &payload, (const uint8_t *) term_binary_data(argv[2]), term_binary_size(argv[2]));
  if (result != Z_OK) {
    z_drop(z_move(keyexpr));
    zxp_session_put_options_drop(&put_options);
    return zxp_session_error_tuple(ctx, "z_bytes_copy_from_buf");
  }

  z_put_options_t options;
  z_put_options_default(&options);
  pthread_mutex_lock(&resource->mutex);
  if (!z_internal_session_check(&resource->session)) {
    pthread_mutex_unlock(&resource->mutex);
    z_drop(z_move(payload));
    z_drop(z_move(keyexpr));
    zxp_session_put_options_drop(&put_options);
    if (memory_ensure_free_with_roots(ctx, TUPLE_SIZE(2), argc, argv, MEMORY_CAN_SHRINK) !=
        MEMORY_GC_OK) {
      RAISE_ERROR(OUT_OF_MEMORY_ATOM);
    }
    term response = term_alloc_tuple(2, &ctx->heap);
    term_put_tuple_element(response, 0, ERROR_ATOM);
    term_put_tuple_element(response, 1, session_closed_atom);
    return response;
  }
  result = z_put(z_loan(resource->session), z_loan(keyexpr), z_move(payload),
      &put_options.options);
  pthread_mutex_unlock(&resource->mutex);
  z_drop(z_move(keyexpr));
  zxp_session_put_options_drop(&put_options);
  if (result != Z_OK) {
    return zxp_session_error_tuple(ctx, "z_put");
  }
  return OK_ATOM;
}
