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

struct co_coroutine_obj;
struct co_multi_co_wq;

/**
 * Possible exit code of coroutine
 */
typedef enum co_yield_rv {
	CO_RV_YIELD_TERM,   /** Execution terminated. */
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
	/** Pointer to async called child coroutine */
	struct co_coroutine_obj *child;

} co_coroutine_obj_t;

/**
 * Exec - wait queue objectg
 * Simply encapsulating a pair of queues - exec and wait, for ease since this
 * pattern is used multiple times.
 */
typedef struct co_exec_wait_q {
	co_queue_t exec;
	co_queue_t wait;
} co_exec_wait_q_t;

#define co_co_exec_wait_q_init()                                                                                       \
	(co_exec_wait_q_t) { co_q_init(), co_q_init() }

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
 * Send memory to deallocation by fast queue deallocator
 * @param wq Coroutine work queue pointer
 * @param ptr Memory pointer
 */
#define co_send_to_fast_dealloc(wq, ptr) co_free(ptr)

/**
 * Send memory to deallocation by slow queue deallocator
 * @param wq Coroutine work queue pointer
 * @param ptr Memory pointer
 */
#define co_send_to_slow_dealloc(wq, ptr) co_free(ptr)

/**
 * Send memory to queue deallocator
 * @param wq Coroutine work queue pointer
 * @param is_fast Is it fast or slow queue
 * @param ptr Memory pointer
 */
#define co_send_to_dealloc(wq, ptr, is_fast)                                                                           \
	is_fast ? co_send_to_fast_dealloc(wq, ptr) : co_send_to_slow_dealloc(wq, ptr);

/**
 * Initialize coroutine work queue
 * @param wq Coroutine work queue pointer
 * @param size Size of multi queue to use for input
 * @return 0 or error code
 */
static __inline__ co_errno_t co_multi_co_wq_init(co_multi_co_wq_t *wq, co_size_t size) {
	int rv = co_completion_init(&wq->bell.bell);
	if (rv)
		return rv;
	rv = co_multi_src_q_init(&wq->inputq, size);
	if (rv) {
		co_multi_src_q_destroy(&wq->inputq);
		return rv;
	}
	wq->bell.wake_me_up = co_atom_init(0);
	wq->fastq = co_co_exec_wait_q_init();
	wq->slowq = co_co_exec_wait_q_init();
	return 0;
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
				task = co_queue_peek(&q->exec);
				if (task) {
					co_q_deq(&q->exec);
					goto task_found;
				}
			}

			/* 2. Now the wait queues */
			for (i = 0, q = &wq->fastq; i < 2; ++i, q = &wq->slowq) {
				for_each_co_queue(task, &q->wait) {
					co_coroutine_obj_t *coroutine = container_of(task, co_coroutine_obj_t, qe);
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
			co_coroutine_obj_t *coroutine = container_of(task, co_coroutine_obj_t, qe);
			co_yield_rv_t co_rv;

			b4sleep = 0; /* No sleep for next round */

			co_rv = coroutine->func(coroutine);
			switch (co_rv) {
				case CO_RV_YIELD_TERM:
					co_send_to_dealloc(wq, task, q == &wq->fastq);
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

#endif /*CO_MULTI_CO_WQ_H*/