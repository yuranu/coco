#include "co_coroutines.h"
#include "dep/co_primitive_allocator.h"

co_routine_decl(int, test0, int, x, int, y);

co_routine_body(test0) {
	co_routine_start(test0);

    co_allocator_t a = co_primitive_allocator_init();
    (void)a;

	fprintf(stderr, "Test 0: x = %d, y = %d\n", args->x, args->y);

	++args->x;
	++args->y;

	co_yield_return(10);

	fprintf(stderr, "Test 0: x = %d, y = %d\n", args->x, args->y);

	co_yield_return(20);

	co_yield_break();
}