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

#define __co_add_coroutine(descr, fname, ...)                                                                          \
    {                                                                                                                  \
        __co_ctx_typename(fname) * ctx;

#endif