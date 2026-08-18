#include <erl_nif.h>
#include <zenoh-pico.h>

extern ERL_NIF_TERM zxp_reply_err_from_zp_reply_err(ErlNifEnv *env,
                                                    const z_loaned_reply_err_t *reply_err);
