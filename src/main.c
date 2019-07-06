
#include "co_dispatcher.h"
#include "co_enumerable.h"

#include <stdio.h>

/**
 * container_of macro, same is in kernel, is used for co_queue implementation
 * If it is not defined, define it.
 */
#ifndef container_of
#define container_of(ptr, type, member) ((type *)((void *)ptr - (&((type *)0)->member)))
#endif

/**
 * TODO: Writing this here while WIP, final version shall be inside
 * co_dispatcher module.
 */
int co_root_routine() {
    __co_dispatcher_ctx_t ctx = {.dispatch.term = 0};
    int __co_error = -1;
    if (co_queue_init(ctx.ex_q, 1024))
        goto __co_out;
    if (co_queue_init(ctx.pd_q, 1024))
        goto __co_free_ex_q;
    if (co_sem_init(ctx.dispatch.bell))
        goto __co_free_pd_q;

    __co_error = 0;
    while (!ctx.dispatch.term) {
        /* Dispatch loop */
        /* First, drain ex_q */
        if (!co_queue_empty(ctx.ex_q)) {
            __co_decriptor_t *descr = co_queue_peek(ctx.ex_q);
            co_retval_t rv = descr->func(descr);
            if (rv == CO_RETVAL_DELAY) {
                /* Delayed return. Move this coroutine to ex_q */
                co_queue_enq(ctx.pd_q, descr);
            } else if (rv == CO_RETVAL_TERM) {
                /* This coroutine is over */
                
            }

            co_sem_down(ctx.dispatch.bell);
            continue;
        }
        /* Second, check the pd_q */
        if (!co_queue_empty(ctx.pd_q)) {
            int i;
            for (i = 0; i < ctx.pd_q.sz; ++i) {
                /* Iterate over each element at pd_q. If it is ready - move back to ex_q */
                __co_decriptor_t *descr = co_queue_peek(ctx.pd_q);
                co_queue_deq(ctx.pd_q);
                if (descr->rv_ready) {
                    co_queue_enq(ctx.ex_q, descr);
                    co_sem_down(ctx.dispatch.bell);
                    break;
                } else {
                    co_queue_enq(ctx.pd_q, descr);
                }
            }
        }
        break; /*No more coroutines. This branch is over.*/
    }

    goto __co_free_bell;
/*__co_free_queues:*/
__co_free_bell:
    co_sem_destroy(ctx.dispatch.bell);
__co_free_pd_q:
    co_queue_destroy(ctx.pd_q);
__co_free_ex_q:
    co_queue_destroy(ctx.ex_q);
__co_out:
    return __co_error;
}

/**
 * TEST
 */
co_enumerable_decl(int, test, int, x, int, y);
co_enumerable(test) {
    co_enumerable_start();
    printf("I am about to yield %d %d\n", ctx->in.x, ctx->in.y);
    ctx->in.y++;
    co_yield_return(10);
    printf("I yielded %d %d\n", ctx->in.x, ctx->in.y);
    ctx->in.x++;
    co_yield_return(9);
    printf("Don't expect return now %d %d\n", ctx->in.x, ctx->in.y);
}
co_enumerable_end();

int main() {
    int i;
    co_foreach(i, test, 5, 15) { printf("Coroutine returned %d\n", i); }
    co_foreach_end();
    return 0;
}