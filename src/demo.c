#include "co_coroutines.h"
#include "dep/co_primitive_allocator.h"
#include <stdio.h>
#include <unistd.h>

#define _(x) self->args.x

void *wake_me_up_in_1_sec(void *p) {
	co_coroutine_obj_t *co = (co_coroutine_obj_t *)p;

	usleep(1000000);

	co_coroutine_obj_ring_the_bell(co);

	return NULL;
}

void *wake_me_up_in_01_sec(void *p) {
	co_coroutine_obj_t *co = (co_coroutine_obj_t *)p;

	usleep(100000);

	co_coroutine_obj_ring_the_bell(co);

	return NULL;
}

co_routine_decl(int, test1, int, x, int, y, struct test1_co_obj *, child);
co_routine_decl(int, test0, int, x, int, y, struct test1_co_obj *, test1);

co_decl_locs(self, test1, pthread_t pt);
co_yield_rv_t test1(struct test1_co_obj *self) {
	co_routine_begin(self, test1);

	co_init_locs(self, test1, 0);

	while (_(x) > _(y)) {
		--_(x);
		if (_(x) % 7 == 0) {
			pthread_create(&self->locs->pt, NULL, wake_me_up_in_1_sec, &self->obj);
			co_yield_wait(self);
		}
		co_yield_return(self, _(x));
	}

	if (_(x) > 100)
		co_yield_break();
	else {
		_(child) = co_fork_run(self, test1, 200, 190);
		for_each_yield_return(self, _(child));
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

	_(test1) = co_fork_run(self, test1, .y = 10, .x = 20);

	while_co_yield_await(self, _(test1)) {
		pthread_t pt;
		printf("Test 1 returne %d\n", _(test1)->rv);
		pthread_create(&pt, NULL, wake_me_up_in_1_sec, &self->obj);
		co_pause(self, _(test1));
		co_yield_wait(self);
		co_run(self, _(test1));
		printf("Just print\n");
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