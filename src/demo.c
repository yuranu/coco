#include "co_coroutines.h"
#include "dep/co_primitive_allocator.h"
#include <stdio.h>

co_routine_decl(int, test1, int, x, int, y);
co_routine_decl(/* void */, test2, int, x, int, y);

co_routine_body(test1) {
	co_routine_start(test1);

	printf("Test 1: x = %d, y = %d\n", args->x, args->y);

	++args->x;
	++args->y;

	co_yield_return(10);

	printf("Test 1: x = %d, y = %d\n", args->x, args->y);

	co_yield_return(20);

	co_yield_break();
}

co_routine_body(test2) {
	co_routine_start(test1);

	printf("Test 2: x = %d, y = %d\n", args->x, args->y);

	++args->x;
	++args->y;

	co_yield_return();

	printf("Test 2: x = %d, y = %d\n", args->x, args->y);

	co_yield_return();

	co_yield_break();
}

int main() {
	co_allocator_t a = co_primitive_allocator_init();
	co_multi_co_wq_t wq;

	co_multi_co_wq_init(&wq, 8, &a, &a);

	co_routine_invoke(test1, &wq, 10, 20);
	co_routine_invoke(test2, &wq, 30, 40);

	co_multi_co_wq_loop(&wq);

	co_multi_co_wq_destroy(&wq);

	return 0;
}