#include <erl_nif.h>
#include <stdbool.h>
#include <zenoh-pico.h>

#include "subscriber_option.h"
#include "term.h"

bool zxp_subscriber_options_init(ErlNifEnv *env, ERL_NIF_TERM term, z_subscriber_options_t *options)
{
  ERL_NIF_TERM head;
  ERL_NIF_TERM tail;

  while (enif_get_list_cell(env, term, &head, &tail))
  {
    const ERL_NIF_TERM *tuple;
    int arity;
    if (!enif_get_tuple(env, head, &arity, &tuple) || arity != 2 ||
        !enif_is_identical(tuple[0], allowed_origin_atom))
    {
      return false;
    }

#if Z_FEATURE_LOCAL_SUBSCRIBER == 1
    if (enif_is_identical(tuple[1], session_local_atom))
    {
      options->allowed_origin = Z_LOCALITY_SESSION_LOCAL;
    }
    else if (enif_is_identical(tuple[1], remote_atom))
    {
      options->allowed_origin = Z_LOCALITY_REMOTE;
    }
    else if (enif_is_identical(tuple[1], any_atom))
    {
      options->allowed_origin = Z_LOCALITY_ANY;
    }
    else
    {
      return false;
    }
#else
    (void)options;
    return false;
#endif
    term = tail;
  }

  return enif_is_empty_list(env, term);
}
