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

#include "co_multi_src_q.h"
#include "dep/co_allocator.h"
#include "dep/co_assert.h"
#include "dep/co_queue.h"
#include "dep/co_sync.h"
#include "dep/co_types.h"

/**
 * ### Memory allocation management pitfall
 *
 * Memory allocation may become a bottleneck if poorly implemented. Problem is, that
 * each coroutine start requires creating an object, hence memory allocation.
 *
 * The idea is to delegate all memory allocations to a separate module, that can
 * be adjusted with accordance to specific needs.
 *
 * There is an assumption that root coroutine invocation may be a little slow, and this we
 * will tolerate. When a user invokes coroutine, he knows it will not execute immidiately,
 * rather start in a different thread.
 *
 * If, however, a coroutine invokes inner coroutine, there is no reason for it to be slow,
 * as we are in essentially a single threaded system.
 *
 * Hence we will differenciate between objects allocated by external allocator, and
 * objects allocated from a coroutine context, this allows optimizations to be done
 * based on single threaded execution properties of coroutines.
 *
 */

/**
 * Future returned to coroutine original caller.
 * TODO: Implement full future functionality
 */
typedef void *co_future_t;

struct co_coroutine_obj;
struct co_multi_co_wq;

/**
 * Possible exit code of coroutine
 */
typedef enum co_yield_rv {
	CO_RV_YIELD_BREAK,  /** Execution terminated. */
	CO_RV_YIELD_WAIT,   /** Not terminated, still running asynchronously */
	CO_RV_YIELD_RETURN, /** Terminated cycle, but there are more cycles */
	CO_RV_YIELD_ERROR   /** Unexpected error during the execution */
} co_yield_rv_t;

/**
 * Coroutine instruction pointer
 * Specifies the point from where to resume coroutine
 */
typedef co_size_t co_ipointer_t;

#define CO_IPOINTER_START (0)

/**
 * Generic coroutine object
 */
typedef struct co_coroutine_obj {
	/** Each object is actually a queue element */
	co_queue_e_t qe;
	/** Pointes back to work queue. */
	struct co_multi_co_wq *wq;
	/** Is async execution of give coroutine ready? */
	co_bool_t ready;
	/** The pointer to the coroutine function */
	co_yield_rv_t (*func)(struct co_coroutine_obj *);
	/** Resume position inside coroutine function */
	co_ipointer_t ip;
	/**
	 * Pointer to coroutine parent
	 * In case of coroutine from fast queue, it is always another coroutine, of type co_coroutine_obj
	 * In case of slow queue, it is always object of type future (TODO: implement future use case)
	 */
	void *parent;

} co_coroutine_obj_t;

/**
 * Exec - wait queue objectg
 * Simply encapsulating a pair of queues - exec and wait, for ease since this
 * pattern is used multiple times.
 */
typedef struct co_exec_wait_q {
	co_queue_t exec;
	co_queue_t wait;
	co_allocator_t *alloc;
} co_exec_wait_q_t;

#define co_co_exec_wait_q_init(alloc)                                                                                  \
	(co_exec_wait_q_t) { co_q_init(), co_q_init(), alloc }

/**
 * The coroutines work queue object
 */
typedef struct co_multi_co_wq {
	/*
	 * This allows putting dispatcher to sleep when there is no work to do, then wake up if needed.
	 * This procedure is called ringing the bell.
	 */
	struct {
		co_atom_t wake_me_up; /** Indication that the wq wants to go to sleep */
		co_completion_t bell; /** Completion object - the bell */
	} bell;					  /** Bell encapsulator */

	co_multi_src_q_t inputq; /** Slow input queue - anyone can write here */
	co_exec_wait_q_t fastq;  /** Fast execution - only non root coroutines get here */
	co_exec_wait_q_t slowq;  /** Slow execution - root coroutines coroutines get here*/

	co_bool_t terminate; /** Indicator to terminate the main loop */

} co_multi_co_wq_t;

/**
 * Initialize coroutine work queue
 * @param wq Coroutine work queue pointer
 * @param size Size of multi queue to use for input
 * @return 0 or error code
 */
static __inline__ co_errno_t co_multi_co_wq_init(co_multi_co_wq_t *wq, co_size_t size, co_allocator_t *fast_alloc,
												 co_allocator_t *slow_alloc) {
	int rv = co_completion_init(&wq->bell.bell);
	if (rv)
		return rv;
	rv = co_multi_src_q_init(&wq->inputq, size);
	if (rv) {
		co_completion_destroy(&wq->bell.bell);
		return rv;
	}
	wq->bell.wake_me_up = co_atom_init(0);
	wq->fastq = co_co_exec_wait_q_init(fast_alloc);
	wq->slowq = co_co_exec_wait_q_init(slow_alloc);
	wq->terminate = 0;
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

	for_each_drain_queue(task, &wq->fastq.exec, co_q_peek, co_q_deq) { wq->fastq.alloc->free(wq->fastq.alloc, task); }
	for_each_drain_queue(task, &wq->fastq.wait, co_q_peek, co_q_deq) { wq->fastq.alloc->free(wq->fastq.alloc, task); }

	for_each_drain_queue(task, &wq->slowq.exec, co_q_peek, co_q_deq) { wq->slowq.alloc->free(wq->slowq.alloc, task); }
	for_each_drain_queue(task, &wq->slowq.wait, co_q_peek, co_q_deq) { wq->slowq.alloc->free(wq->slowq.alloc, task); }

	for_each_drain_queue(task, &wq->inputq, co_multi_src_q_peek, co_multi_src_q_deq) {
		wq->slowq.alloc->free(wq->slowq.alloc, task);
	}

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
	int i = 0;
	while (!wq->terminate) {
		co_queue_e_t *task = NULL; /* Attempt to get a new taks */
		co_exec_wait_q_t *q;
		do {
			/* 1. Start with draining the exec queues */
			for (i = 0, q = &wq->fastq; i < 2; ++i, q = &wq->slowq) {
				task = co_q_peek(&q->exec);
				if (task) {
					co_q_deq(&q->exec);
					goto task_found;
				}
			}

			/* 2. Now the wait queues */
			for (i = 0, q = &wq->fastq; i < 2; ++i, q = &wq->slowq) {
				for_each_co_queue(task, &q->wait) {
					co_coroutine_obj_t *coroutine = __co_container_of(task, co_coroutine_obj_t, qe);
					if (coroutine->ready) {
						co_q_deq(&q->wait);
						goto task_found;
					}
				}
			}

			/* 3. Now the input queue */
			q = &wq->slowq;
			task = co_multi_src_q_peek(&wq->inputq);
			if (task) {
				co_multi_src_q_deq(&wq->inputq);
				goto task_found;
			}

			/* 4. If we are here - we found nothing */
			if (!b4sleep) {
				b4sleep = 1;
				co_atom_set(&wq->bell.wake_me_up, 1); /* Warn everyone we are going to sleep soon */
			} else {
				/* Go to sleep */
				co_errno_t err = co_completion_wait(&wq->bell.bell);
				co_assert(!err || err == -EINTR, "Unexpected error while during completion wait %d\n", err);
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

			co_rv = coroutine->func(coroutine);
			switch (co_rv) {
				case CO_RV_YIELD_BREAK:
					q->alloc->free(q->alloc, task);
					break;
				case CO_RV_YIELD_WAIT:
					co_q_enq(&q->wait, task);
					break;
				case CO_RV_YIELD_RETURN:
					co_q_enq(&q->exec, task);
					break;
				case CO_RV_YIELD_ERROR:
					co_assert(0, "Unexpected error returned from coroutine\n");
			}
		}
	}
	return 0;
}

/**
 * Activate the bell event of work queue
 * @param co Coroutine object pointer
 */
void static __inline__ co_coroutine_obj_ring_the_bell(co_coroutine_obj_t *co) {
	if (co_atom_cmpxchg(&co->wq->bell.wake_me_up, 1, 0) == 1)
		co->ready = 1;
		co_completion_done(&co->wq->bell.bell);
}

#endif /*CO_MULTI_CO_WQ_H*/