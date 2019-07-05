#include <stdio.h>
#include "co_enumerable.h"

/**
 * TEST
 */
co_enumerable_decl(int, test, (int, x, int, y));
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