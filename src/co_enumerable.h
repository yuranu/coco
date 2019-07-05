#ifndef CO_ENUMERABLE_H
#define CO_ENUMERABLE_H

#include "mem/co_alloc.h"
#include "utils/co_macro.h"

typedef enum co_retval { CO_RETVAL_TERM, CO_RETVAL_RETURN, CO_RETVAL_DELAY } co_retval_t;

#define CO_ZERO_STATE 0

#define __co_ctx_typename(fname) __co_ctx_##fname##_t

#define __co_ctx_def(retval, fname, args)                                                                              \
    typedef struct {                                                                                                   \
        struct {                                                                                                       \
            int state;                                                                                                 \
        } __co_sys;                                                                                                    \
        struct {                                                                                                       \
            __co_pairs(;, __co_list_c args);                                                                           \
        } in;                                                                                                          \
        retval rv;                                                                                                     \
    } __co_ctx_typename(fname)

#define __co_enumerable_proto(fname) co_retval_t fname(__co_ctx_typename(fname) * ctx)

#define co_enumerable_decl(retval, fname, args)                                                                        \
    __co_ctx_def(retval, fname, args);                                                                                 \
    __co_enumerable_proto(fname)

#define co_enumerable(fname)                                                                                           \
    __co_enumerable_proto(fname) {                                                                                     \
        switch (ctx->__co_sys.state) {

#define co_enumerable_start()                                                                                          \
    case CO_ZERO_STATE:                                                                                                \
        __co_nop()

#define co_enumerable_end()                                                                                            \
    }                                                                                                                  \
    co_yield_break();                                                                                                  \
    }

#define co_yield_break() return CO_RETVAL_TERM

#define co_yield_return(x)                                                                                             \
    ctx->__co_sys.state = __LINE__;                                                                                    \
    ctx->rv = x;                                                                                                       \
    return CO_RETVAL_RETURN;                                                                                           \
    case __LINE__:                                                                                                     \
        __co_nop()

#define co_foreach(var, fname, ...)                                                                                    \
    {                                                                                                                  \
        __co_ctx_typename(fname) * ctx;                                                                                \
        ctx = co_ctx_alloc(ctx);                                                                                       \
        *ctx = (__co_ctx_typename(fname)){.__co_sys.state = CO_ZERO_STATE, .in = {__co_list_c(__VA_ARGS__)}};          \
        while (1) {                                                                                                    \
            const co_retval_t __co_retval = fname(ctx);                                                                \
            if (__co_retval == CO_RETVAL_TERM)                                                                         \
                break;                                                                                                 \
            var = ctx->rv;

#define co_foreach_end()                                                                                               \
    }                                                                                                                  \
    free(ctx);                                                                                                         \
    }

#endif /*CO_ENUMERABLE_H*/