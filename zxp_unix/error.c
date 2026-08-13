#include <erl_nif.h>

ERL_NIF_TERM error_reason_tuple(ErlNifEnv *env, const char *str)
{

  ErlNifBinary bin;
  enif_alloc_binary(strlen(str), &bin);
  memcpy(bin.data, str, strlen(str));

  return enif_make_tuple2(env, enif_make_atom(env, "error"), enif_make_binary(env, &bin));
}
