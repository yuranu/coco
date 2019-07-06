#ifndef CO_DISPATCHER_H
#define CO_DISPATCHER_H

#include "co_enumerable.h"
#include "utils/co_queue.h"

typedef struct __co_dispatcher_ctx {

    /**
     * Public dispatcher object
     */
    co_dispatcher_t dispatch;

    /**
     * Execution queue. Each new coroutine arrives here.
     */
    co_queue_define(ex_q, __co_decriptor_t);

    /**
     * Pending queue. Coroutine gets here when it returns with rv not read.
     */
    co_queue_define(pd_q, __co_decriptor_t);
} __co_dispatcher_ctx_t;

#define __co_start_coroutine(disp, fname, ...)                                                                         \
    ({                                                                                                                 \
        int __err_ = 0;                                                                                                \
        __co_ctx_typename(fname) * __co_ctx;                                                                           \
        __co_ctx = co_ctx_alloc(__co_ctx);                                                                             \
        if (!__co_ctx) {                                                                                               \
            __err_ = -ENOMEM;                                                                                          \
        } else {                                                                                                       \
            *__co_ctx = __co_init_ctx(fname, (disp), ##__VA_ARGS__);                                                   \
            __err_ = co_queue_enq(container_of((disp), __co_dispatcher_ctx_t, dispatch)->ex_q, &__co_ctx->__co_sys);   \
            if (__err_) {                                                                                              \
                co_ctx_free(__co_ctx);                                                                                 \
            } else {                                                                                                   \
                __err_ = co_ring_the_bell((disp));                                                                     \
                if (__err_)                                                                                            \
                    co_ctx_free(__co_ctx);                                                                             \
            }                                                                                                          \
        }                                                                                                              \
        __err_;                                                                                                        \
    })

#endif