#ifndef CO_ENUMERABLE_H
#define CO_ENUMERABLE_H

#include "co_dispatcher.h"
#include "sys/co_alloc.h"
#include "utils/co_macro.h"

/**
 * Possible exit status for corotine
 */
typedef enum co_retval {
    CO_RETVAL_TERM /** Terminated, no return value */,
    CO_RETVAL_RETURN /** Yielded, with return value */,
} co_retval_t;

#define __CO_ZERO_STATE 0

#define __co_ctx_typename(fname) __co_ctx_##fname##_t

#define __co_ctx_def(retval, fname, ...)                                                                               \
    typedef struct {                                                                                                   \
        __co_decriptor_t __co_sys;                                                                                     \
        struct {                                                                                                       \
            __co_pairs(;, ##__VA_ARGS__);                                                                              \
        } in;                                                                                                          \
        retval rv;                                                                                                     \
    } __co_ctx_typename(fname)

#define __co_enumerable_proto(fname) co_retval_t fname(__co_ctx_typename(fname) * ctx)

/**
 * Declare coroutine
 * @param retval Coroutine return value
 * @param fname Coroutine name
 * @param ... Optional comma separated list of function arguments [type, name, type, name, ...]
 */
#define co_enumerable_decl(retval, fname, ...)                                                                         \
    __co_ctx_def(retval, fname, ##__VA_ARGS__);                                                                        \
    __co_enumerable_proto(fname)

/**
 * Start coroutine, previously declared
 */
#define co_enumerable(fname)                                                                                           \
    __co_enumerable_proto(fname) {                                                                                     \
        switch (ctx->__co_sys.state) {

/**
 * Must come as a first statement in coroutine body
 */
#define co_enumerable_start()                                                                                          \
    case __CO_ZERO_STATE:                                                                                              \
        __co_nop()

/**
 * End coroutine body
 */
#define co_enumerable_end()                                                                                            \
    }                                                                                                                  \
    co_yield_break();                                                                                                  \
    }

/**
 * Terminate coroutine, no return value
 */
#define co_yield_break() return CO_RETVAL_TERM

/**
 * Yield coroutine, return @x
 */
#define co_yield_return(x)                                                                                             \
    ctx->__co_sys.state = __LINE__;                                                                                    \
    ctx->__co_sys.rv_ready = 1;                                                                                        \
    ctx->rv = x;                                                                                                       \
    return CO_RETVAL_RETURN;                                                                                           \
    case __LINE__:                                                                                                     \
        __co_nop()

/**
 * Call coroutine @fname while it is not terminated. Store result in
 */
#define co_foreach(var, fname, ...)                                                                                    \
    {                                                                                                                  \
        __co_ctx_typename(fname) * ctx;                                                                                \
        ctx = co_ctx_alloc(ctx);                                                                                       \
        *ctx = (__co_ctx_typename(fname)){.__co_sys.state = __CO_ZERO_STATE, .in = {__co_list_c(__VA_ARGS__)}};        \
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