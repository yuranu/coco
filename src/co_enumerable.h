#ifndef CO_ENUMERABLE_H
#define CO_ENUMERABLE_H

#include "sys/co_alloc.h"
#include "sys/co_threads.h"
#include "utils/co_macro.h"

/**
 * Possible exit status for corotine
 */
typedef enum co_retval {
    CO_RETVAL_TERM /** Terminated, no return value */,
    CO_RETVAL_RETURN /** Yielded, with return value */,
    CO_RETVAL_DELAY /** Yielded, with delayed return value */,
    CO_RETVAL_ERROR /** Error while creating new coroutine */
} co_retval_t;

#define __CO_ZERO_STATE 0

/**
 * Coroutines dispatcher descriptor, exposed to the coroutines.
 * Shall be one object per dispatcher thread.
 */
typedef struct co_dispatcher {
    /**
     * Each coroutine waiting to be execute rings the bell.
     * Each coroutine that returns with CO_RETVAL_DELAY is moved to
     * pending queue. As soon as it finally has the result, it rings the bell.
     */
    co_sem_define(bell);

    /**
     * Trigger for dispatcher to terminate
     */
    volatile int term;
} co_dispatcher_t;

/**
 * Notify dispatcher about some sort of event
 */
#define co_ring_the_bell(dispatch) co_sem_up((dispatch)->bell)

/**
 * Coroutine descriptor
 */
typedef struct __co_decriptor {
    co_dispatcher_t *dispatch;
    co_retval_t (*func)(void *);
    int state;
    int rv_ready;
} __co_decriptor_t;

#define __co_ctx_typename(fname) __co_ctx_##fname##_t

#define __co_ctx_def(retval, fname, ...)                                                                               \
    typedef struct {                                                                                                   \
        __co_decriptor_t __co_sys;                                                                                     \
        struct {                                                                                                       \
            __co_pairs(;, ##__VA_ARGS__);                                                                              \
        } in;                                                                                                          \
        retval rv;                                                                                                     \
    } __co_ctx_typename(fname)

#define __co_init_ctx(fname, disp, ...)                                                                                \
    (__co_ctx_typename(fname)){.__co_sys.dispatch = disp,                                                              \
                               .__co_sys.func = fname,                                                                 \
                               .__co_sys.state = __CO_ZERO_STATE,                                                      \
                               .__co_sys.rv_ready = 0,                                                                 \
                               .in = {__VA_ARGS__}};

#define __co_enumerable_proto(fname) co_retval_t fname(void *__ctx)

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
        __co_ctx_typename(fname) *ctx = (__co_ctx_typename(fname) *)__ctx;                                             \
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