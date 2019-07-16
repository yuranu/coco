#include "co_coroutines.h"
#include "dep/co_primitive_allocator.h"
#include <stdio.h>
#include <unistd.h>

#define _(x) self->args.x

co_routine_decl(int, test1, int, x, int, y);
co_routine_decl(int, test0, int, x, int, y, struct test1_co_obj *, test1);
// co_routine_decl(/* void */, test2, int, x, int, y);
// co_routine_decl(int, test3, int, i);

co_yield_rv_t test1(struct test1_co_obj *self) {
	co_routine_begin(self, test1);

	while (_(x) > _(y)) {
		--_(x);
		co_yield_return(self, _(x));
	}
	co_yield_break();
}

co_yield_rv_t test0(struct test0_co_obj *self) {
	co_routine_begin(self, test0);

	printf("test0: %d %d\n", _(x), _(y));

	co_yield_return(self, 10);

	self->args.x += 1;
	self->args.y += 2;

	printf("test0: %d %d\n", _(x), _(y));

	co_yield_return(self, 20);

	_(test1) = co_fork(self, test1, 20, 10);

	co_run(self, _(test1));

	while_co_yield_await(self, _(test1)) {
		printf("Test 1 returne %d\n", _(test1)->rv);

		co_yield_await_next(self, _(test1));
	}

	co_yield_break();
}

int main() {
	co_allocator_t a = co_primitive_allocator_init();
	co_multi_co_wq_t wq;
	struct test0_co_obj *test0co;

	co_multi_co_wq_init(&wq, 8, &a, &a);

	test0co = co_new(&wq, test0, 10, 10, NULL);

	co_schedule(&wq, test0co);

	// co_routine_invoke(test1, &wq, 10, 20);
	// co_routine_invoke(test2, &wq, 30, 40);

	co_multi_co_wq_loop(&wq);

	co_multi_co_wq_destroy(&wq);

	return 0;
}