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
#define co_multi_co_wq_alloc(wq, type, size) (wq)->type##_alloc->alloc((wq)->type##_alloc, size)
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
		do {
			co_size_t initial_size = wq->execq.count;
			int i;
			/* 1. Start with draining the exec queues */
			for (i = 0; i < initial_size; ++i) {
				co_list_e_t *task             = co_q_peek(&wq->execq); /* Attempt to get a new taks */
				co_coroutine_obj_t *coroutine = __co_container_of(task, co_coroutine_obj_t, qe); /* Extract coroutine */
				co_yield_rv_t co_rv;

				co_q_deq(&wq->execq);

				if (co_is_terminated(coroutine)) {
					co_dbg_trace("Coroutine <%s> is terminated, freeing\n", coroutine->func_name);
					co_multi_co_wq_free(wq, task); /* Free */
					break;                         /* Next taks */
				}
				co_dbg_trace("Going to call <%s>\n", coroutine->func_name);
				co_rv = coroutine->func(coroutine);
				co_dbg_trace("Call result: <%d>\n", co_rv);
				switch (co_rv) {
					case CO_RV_YIELD_RETURN:
					case CO_RV_YIELD_BREAK:
						while (coroutine->await) { /* If it is a child coroutine, reschedule its parents. */
							co_list_e_t *pending = coroutine->await;
							coroutine->await     = pending->next;
							co_q_enq(&wq->execq, pending);
						}
						co_q_enq(&wq->execq, task);     /* Reschedule itself */
						if (co_rv == CO_RV_YIELD_BREAK) /* If needed mark for erase */
							co_routine_flag_set(&coroutine->flags, CO_FLAG_TERM);
						goto break_loop;
					case CO_RV_YIELD_AWAIT:
						/* Do nothing, not my responsibility now */
						goto break_loop;
					case CO_RV_YIELD_COND_WAIT:
						co_q_enq(&wq->execq, task); /* Simply reschedule to re-test later, try next task */
						/* Do not reset b4sleep */
						break;
					case CO_RV_YIELD_ERROR:
						co_assert(0, "Unexpected error returned from coroutine\n");
						break;
				}
			}

		break_loop:

			/* Do not continue to read input if we may have more stuff to do */
			if (i < initial_size) {
				b4sleep = 0;
				break;
			}
			/* Else - done nothing this turn, try new inputs */

			/* 2. Now the input queue */ {
				co_list_e_t *task = co_multi_src_q_peek(&wq->inputq);
				if (task) {
					co_multi_src_q_deq(&wq->inputq);
					co_q_enq(&wq->execq, task);
					break; /* Don't continue any further - analyze what we have */
				}
			}

			/* 3. If we are here - we found nothing */
			if (!b4sleep) {
				co_dbg_trace("Work queue <%p> is feeling sleepy\n", wq);
				b4sleep = 1;
				co_atom_set(&wq->bell.wake_me_up, 1); /* Warn everyone we are going to sleep soon */
			} else {
				/* Go to sleep */
				co_errno_t err;
				co_dbg_trace("Work queue <%p> is going to sleep\n", wq);
				err = co_completion_wait(&wq->bell.bell);
				(void)err;
				co_assert(!err || err == -EINTR, "Unexpected error while during completion wait %d\n", err);
				co_dbg_trace("Work queue <%p> is awake\n", wq);
				/* Good morning beautiful */
				b4sleep = 0;
			}
		} while (0);
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