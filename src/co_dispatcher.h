#ifndef CO_DISPATCHER_H
#define CO_DISPATCHER_H

#include "sys/co_threads.h"
#include "utils/co_queue.h"

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
} co_dispatcher_t;

typedef struct __co_decriptor {
    co_dispatcher_t *dispatch;
    int state;
    int rv_ready;
} __co_decriptor_t;

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
     * Pending queue. Each new coroutine gets here.
     */
    co_queue_define(pd_q, __co_decriptor_t);
} __co_dispatcher_ctx_t;

#endif