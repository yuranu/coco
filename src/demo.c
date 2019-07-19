#include "co_coroutines.h"
#include "co_shortcuts.h"
#include "dep/co_primitive_allocator.h"
#include "dep/co_timeout.h"
#include <stdio.h>
#include <unistd.h>

/* Sample 1 */
co_routine_decl(int, fibonacci_producer, int, x, int, y);
co_routine_decl(/*void*/, fibonacci_printer, struct fibonacci_producer_co_obj *, producer);

co_yield_rv_t fibonacci_producer(struct fibonacci_producer_co_obj *self) {
	co_routine_begin(self, fibonacci_producer);
	while (1) {
		_(y) = _(x) + _(y);
		_(x) = _(y) - _(x);
		co_yield_return(self, _(x));
	}
	co_yield_break();
}

co_yield_rv_t fibonacci_printer(struct fibonacci_printer_co_obj *self) {
	co_routine_begin(self, fibonacci_printer);
	_(producer) = co_fork_run(self, fibonacci_producer, 0, 1);
	while_co_yield_await(self, _(producer)) {
		printf("Got result from fibonacci producer: %d\n", _(producer)->rv);
		if (_(producer)->rv > 100) {
			printf("Got enough fibonacci numbers.\n");
			break;
		}
		co_pause(self, _(producer));
		co_yield_wait_timeout(self, 1000000000UL);
		co_run(self, _(producer));
	}
	co_force_terminate(&_(producer)->obj);
	co_yield_break();
}

/* Sample 2 */
co_routine_decl(const char *, data_source_1, int, i);
co_routine_decl(const char *, data_source_2, int, i);
co_routine_decl(const char *, data_collector, struct data_source_1_co_obj *, d1, struct data_source_2_co_obj *, d2);
co_routine_decl(/*void*/, data_printer, struct data_collector_co_obj *, d);

co_yield_rv_t data_source_1(struct data_source_1_co_obj *self) {
	char *data[] = {"Hello.", "Is it me", "you're", "looking for?"};
	co_routine_begin(self, data_source_1);
	while (_(i) < sizeof(data) / sizeof(*data)) {
		co_yield_return(self, data[_(i)]);
		++_(i);
	}
	co_yield_break();
}

co_yield_rv_t data_source_2(struct data_source_2_co_obj *self) {
	char *data[] = {"Ohhhh", "Mama mia", "Mama mia", "Mama mia", "Let him go!"};
	co_routine_begin(self, data_source_2);
	while (_(i) < sizeof(data) / sizeof(*data)) {
		co_yield_return(self, data[_(i)]);
		++_(i);
	}
	co_yield_break();
}

co_yield_rv_t data_collector(struct data_collector_co_obj *self) {
	co_routine_begin(self, data_collector);
	_(d1) = co_fork_run(self, data_source_1, 0);
	for_each_yield_return(self, _(d1));
	_(d2) = co_fork_run(self, data_source_2, 0);
	for_each_yield_return(self, _(d2));
	co_yield_break();
}

co_yield_rv_t data_printer(struct data_printer_co_obj *self) {
	co_routine_begin(self, data_printer);
	_(d) = co_fork_run(self, data_collector, 0);
	while_co_yield_await(self, _(d)) {
		printf("%s\n", _(d)->rv);
		co_pause(self, _(d));
		co_yield_wait_timeout(self, 200000000UL);
		co_run(self, _(d));
	}
	co_yield_break();
}

void *async_co_scheduler(void *param) {
	co_multi_co_wq_t *wq = (co_multi_co_wq_t *)param;
	/* Test: schedule coroutine asynchronously from a separate thread */
	struct data_printer_co_obj *d;
	d = co_new(wq, data_printer);
	usleep(5000000); /* Sleep before schedule to make sure wq is already in the middle of something */
	printf("Scheduling a new one\n");
	co_schedule(wq, d);
	return NULL;
}

int main() {
	co_allocator_t a = co_primitive_allocator_init();
	co_multi_co_wq_t wq;
	struct fibonacci_printer_co_obj *fp;
	pthread_t sched;

	co_multi_co_wq_init(&wq, 8, &a, &a);

	fp = co_new(&wq, fibonacci_printer);

	pthread_create(&sched, NULL, async_co_scheduler, &wq);

	co_schedule(&wq, fp);

	/* Run the loop */
	co_multi_co_wq_loop(&wq);

	/* Cleanup */
	co_multi_co_wq_destroy(&wq);
	pthread_join(sched, NULL);

	return 0;
}