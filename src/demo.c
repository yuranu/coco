#include "co_coroutines.h"
#include "dep/co_primitive_allocator.h"
#include <stdio.h>
#include <unistd.h>

co_routine_decl(int, test1, int, x, int, y);
co_routine_decl(int, test0, int *, x, int, y);
co_routine_decl(/* void */, test2, int, x, int, y);

void *waker_thread(void *param) {
	usleep(1000000);
	co_coroutine_obj_ring_the_bell(((co_coroutine_obj_t *)param));
	return NULL;
}

void *waker_thread2(void *param) {
	usleep(2000000);
	co_coroutine_obj_ring_the_bell(((co_coroutine_obj_t *)param));
	return NULL;
}

co_routine_body(test0) {
	co_routine_start(test0);

	printf("Test 0: x = %d, y = %d\n", *args->x, args->y);

	++(*args->x);
	++args->y;

	co_yield_return(10);

	{
		pthread_t sleeper;
		pthread_create(&sleeper, NULL, waker_thread2, __co_obj);
	}
	co_yield_wait();

	printf("Test 0: x = %d, y = %d\n", *args->x, args->y);

	co_yield_return(20);

	co_yield_break();
}

co_routine_body(test1) {
	int test0retval;
	co_routine_start(test1);

	printf("Test 1: x = %d, y = %d\n", args->x, args->y);

	++args->x;
	++args->y;

	co_yield_return(10);

	co_foreach_yield(test0retval, test0, (&args->x, args->y), {
		printf("Test 1: got response %d from test 0\n", test0retval);
	});

	printf("Test 1: x = %d, y = %d\n", args->x, args->y);

	co_yield_return(20);

	co_yield_break();
}

co_routine_body(test2) {
	co_routine_start(test2);

	printf("Test 2: x = %d, y = %d\n", args->x, args->y);

	++args->x;
	++args->y;

	co_yield_return();
	{
		pthread_t sleeper;
		pthread_create(&sleeper, NULL, waker_thread, __co_obj);
	}
	co_yield_wait();

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