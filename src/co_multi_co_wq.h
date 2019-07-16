#ifndef CO_MULTI_CO_WQ_H
#define CO_MULTI_CO_WQ_H
/**
 * @file co_multi_co_wq.h

 * This is the heart of the coroutine system.
 *
 * Work queue / coroutines dispatcher.
 * Accepts inmputs from multiple processes in form of coroutine objects,
 * and execute them.
 *
 */

#include "co_coroutine_object.h"
#include "co_multi_src_q.h"
#include "dep/co_allocator.h"
#include "dep/co_dbg.h"
#include "dep/co_list.h"
#include "dep/co_sync.h"
#include "dep/co_types.h"

/**
 * The coroutines work queue object
 */
typedef struct co_multi_co_wq {
	/*
	 * This allows putting dispatcher to sleep when there is no work to do, then wake up if needed.
	 * This procedure is called ringing the bell.
	 */

	/** Bell encapsulator */
	struct {
		/** Indication that the wq wants to go to sleep */
		co_atom_t wake_me_up;
		/** Completion object - the bell */
		co_completion_t bell;
	} bell;

	/** Slow input queue - anyone can write here */
	co_multi_src_q_t inputq;
	struct {
		/** Fast allocator - for allocation from coroutine wq thread, can be lockless */
		co_allocator_t *fast_alloc;
		/** Slow allocator - for allocation from any thread, must have locks */
		co_allocator_t *slow_alloc;
	};
	struct {
		/** Execution queue */
		co_queue_t execq;
		/** Wait queueu */
		co_queue_t waitq;
	};
	/** Indicator to terminate the main loop */
	co_bool_t terminate;
} co_multi_co_wq_t;

/**
 * Allocate memory on given allocator
 * @param wq Coroutine work queue pointer
 * @param type Either `fast` or `slow`
 * @param size Memory size
 * @return Allocated pointer or NULL
 */
#define co_multi_co_wq_alloc(wq, type, size) wq->type##_alloc->alloc(wq->type##_alloc, size)
#define co_multi_co_wq_alloc_fast(wq, size) co_multi_co_wq_alloc(wq, fast, size)
#define co_multi_co_wq_alloc_slow(wq, size) co_multi_co_wq_alloc(wq, slow, size)

/**
 * Free memory allocated for coroutine
 * @param wq Coroutine work queue pointer
 * @param task Queue element of coroutine wq representing a coroutine
 */
static __inline__ void co_multi_co_wq_free(co_multi_co_wq_t *wq, co_list_e_t *task) {
	if (co_routine_flag_test(__co_container_of(task, co_coroutine_obj_t, qe)->flags, CO_FLAG_SLOW_ALLOC))
		wq->slow_alloc->free(wq->slow_alloc, task);
	else
		wq->fast_alloc->free(wq->slow_alloc, task);
}

/**
 * Initialize coroutine work queue
 * @param wq Coroutine work queue pointer
 * @param size Size of multi queue to use for input
 * @return 0 or error code
 */
static __inline__ co_errno_t co_multi_co_wq_init(co_multi_co_wq_t *wq, co_size_t size, co_allocator_t *fast_alloc,
                                                 co_allocator_t *slow_alloc) {
	int rv;
	*wq = (co_multi_co_wq_t){.bell.wake_me_up = co_atom_init(0),
	                         .execq           = co_q_init(),
	                         .waitq           = co_q_init(),
	                         .fast_alloc      = fast_alloc,
	                         .slow_alloc      = slow_alloc,
	                         .terminate       = 0};
	rv  = co_completion_init(&wq->bell.bell);
	if (rv)
		return rv;
	rv = co_multi_src_q_init(&wq->inputq, size);
	if (rv) {
		co_completion_destroy(&wq->bell.bell);
		return rv;
	}
	return 0;
}

/**
 * Destroy work queue
 * Deallocate all objects in the queue before that
 * @param wq Coroutine work queue pointer
 * @warning New calls arriving during destruction is undefined behaviour
 */
static __inline__ void co_multi_co_wq_destroy(co_multi_co_wq_t *wq) {
	void *task;

	for_each_drain_queue(task, &wq->execq, co_q_peek, co_q_deq) { co_multi_co_wq_free(wq, task); }
	for_each_drain_queue(task, &wq->waitq, co_q_peek, co_q_deq) { co_multi_co_wq_free(wq, task); }

	for_each_drain_queue(task, &wq->inputq, co_multi_src_q_peek, co_multi_src_q_deq) { co_multi_co_wq_free(wq, task); }

	co_multi_src_q_destroy(&wq->inputq);
	co_completion_destroy(&wq->bell.bell);
}

/**
 * Loop in coroutine work queue loop, until terminated.
 * @param wq Coroutine work queue pointer
 * @param size Size of multi queue to use for input
 * @return 0 or error code
 */
static __inline__ co_errno_t co_multi_co_wq_loop(co_multi_co_wq_t *wq) {
	co_bool_t b4sleep = 0; /* Whether or not we are planning to go to sleep next round */
	while (!wq->terminate) {
		co_list_e_t *task = NULL; /* Attempt to get a new taks */
		do {

			/* 1. Start with draining the exec queues */
			task = co_q_peek(&wq->execq);
			if (task) {
				co_q_deq(&wq->execq);
				goto task_found;
			}

			/* 2. Now the wait queues */
			co_list_e_t *prev;
			for_each_filter_list(task, prev, &wq->waitq) {
				co_coroutine_obj_t *coroutine = __co_container_of(task, co_coroutine_obj_t, qe);
				if (co_routine_flag_test(coroutine->flags, CO_FLAG_READY)) { /* Great, pending task became ready */
					co_q_cherry_pick(&wq->waitq, task, prev);
					goto task_found;
				}
			}

			/* 3. Now the input queue */
			task = co_multi_src_q_peek(&wq->inputq);
			if (task) {
				co_multi_src_q_deq(&wq->inputq);
				goto task_found;
			}

			/* 4. If we are here - we found nothing */
			if (!b4sleep) {
				co_dbg_trace("Work queue <%p> feeling sleepy\n", wq);
				b4sleep = 1;
				co_atom_set(&wq->bell.wake_me_up, 1); /* Warn everyone we are going to sleep soon */
			} else {
				/* Go to sleep */
				co_errno_t err = co_completion_wait(&wq->bell.bell);
				(void)err;
				co_dbg_trace("Work queue <%p> is going to sleep\n", wq);
				co_assert(!err || err == -EINTR, "Unexpected error while during completion wait %d\n", err);
				co_dbg_trace("Work queue <%p> is awake\n", wq);
				/* Good morning beautiful */
				b4sleep = 0;
			}
		} while (0);
	task_found:
		if (task != NULL) {
			/* Attempt to execute the task */
			co_coroutine_obj_t *coroutine = __co_container_of(task, co_coroutine_obj_t, qe);
			co_yield_rv_t co_rv;

			b4sleep = 0; /* No sleep for next round */

			co_dbg_trace("Going to call <%s>\n", coroutine->func_name);
			co_rv = coroutine->func(coroutine);
			co_dbg_trace("Call result: <%d>\n", co_rv);

			switch (co_rv) {
				case CO_RV_YIELD_BREAK:
					if (coroutine->await) { /* If it is a child coroutine, notify parent it terminated */
						coroutine->await->child = NULL;
						co_routine_flag_set(&coroutine->await->flags, CO_FLAG_READY);
					}
					co_multi_co_wq_free(wq, task);
					break;
				case CO_RV_YIELD_WAIT:
					co_q_enq(&wq->waitq, task); /* Just move to wait queue */
					break;
				case CO_RV_YIELD_RETURN:
					if (coroutine->await) /* If it is a child coroutine, notify parent it has intermediate results */
						co_routine_flag_set(&coroutine->await->flags, CO_FLAG_READY);
					else
						co_q_enq(&wq->execq, task); /* Else reschedule */
					break;
				case CO_RV_YIELD_ERROR:
					co_assert(0, "Unexpected error returned from coroutine\n");
				case CO_RV_YIELD_WAIT_COND:
					co_assert(0, "Not implemented yet\n");
			}
		}
	}
	return 0;
}

/**
 * Activate the bell event of work queue
 * @param co Coroutine work queue pointer
 */
void static __inline__ co_multi_co_wq_ring_the_bell(co_multi_co_wq_t *wq) {
	if (co_atom_cmpxchg(&wq->bell.wake_me_up, 1, 0) == 1)
		co_completion_done(&wq->bell.bell);
}

#endif /*CO_MULTI_CO_WQ_H*/