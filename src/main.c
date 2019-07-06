
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

void dispatcher_test() {
    int rv;
    __co_dispatcher_ctx_t ctx;
    rv = co_queue_init(ctx.ex_q, 1024);
}

int main() {
    int i;
    co_foreach(i, test, 5, 15) { printf("Coroutine returned %d\n", i); }
    co_foreach_end();
    return 0;
}