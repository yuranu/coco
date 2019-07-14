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
#include "dep/co_dbg.h"
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
 * @todo Implement full future functionality
 */
typedef struct co_future {
} co_future_t;

struct co_coroutine_obj;
struct co_multi_co_wq;

/**
 * Possible exit code of coroutine
 */
typedef enum co_yield_rv {
	/** Execution terminated. */
	CO_RV_YIELD_BREAK,
	/** Not terminated, still running asynchronously */
	CO_RV_YIELD_WAIT,
	/** Terminated cycle, but there are more cycles */
	CO_RV_YIELD_RETURN,
	/** Unexpected error during the execution */
	CO_RV_YIELD_ERROR
} co_yield_rv_t;

/**
 * Coroutine instruction pointer
 * Specifies the point from where to resume coroutine
 */
typedef co_size_t co_ipointer_t;

#define CO_IPOINTER_START (0)

/**
 * Coroutine flags bitmap
 */
typedef int co_routine_flags_bmp_t;

/**
 * All possible coroutine flags values
 */
typedef enum co_routine_flag {
	/** Execution results are ready */
	CO_FLAG_READY,
	/** Corotine object was created by slow allocator */
	CO_FLAG_SLOW_ALLOC
} co_routine_flag_t;

#define co_routine_flags_init() ((co_routine_flags_bmp_t)0)

static __inline__ int co_routine_flag_test(co_routine_flags_bmp_t flags, co_routine_flag_t flag) {
	return flags & (1 << flag);
}
static __inline__ void co_routine_flag_set(co_routine_flags_bmp_t *flags, co_routine_flag_t flag) {
	*flags |= (1 << flag);
}
static __inline__ void co_routine_flag_clear(co_routine_flags_bmp_t *flags, co_routine_flag_t flag) {
	*flags &= ~(1 << flag);
}

/**
 * Generic coroutine object
 */
typedef struct co_coroutine_obj {
	/** Each object is actually a queue element */
	co_queue_e_t qe;
	/** Pointes back to work queue. */
	struct co_multi_co_wq *wq;
	/** The pointer to the coroutine function */
	co_yield_rv_t (*func)(struct co_coroutine_obj *);
	/** Resume position inside coroutine function */
	co_ipointer_t ip;
	/** Bitmap with various co_routine flags */
	co_routine_flags_bmp_t flags;
	/** Pointer to a child coroutine */
	struct co_coroutine_obj *child;
	union {
		/** Pointer to the parent coroutine for sub-routines only */
		struct co_coroutine_obj *parent;
		/** Pointer to future object for top level only routines*/
		co_future_t *future;
	};
	/** Debug only trace function name */
	co_dbg(const char *func_name);
} co_coroutine_obj_t;

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
static __inline__ void co_multi_co_wq_free(co_multi_co_wq_t *wq, co_queue_e_t *task) {
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
		co_queue_e_t *task = NULL; /* Attempt to get a new taks */
		do {
			/* 1. Start with draining the exec queues */
			task = co_q_peek(&wq->execq);
			if (task) {
				co_q_deq(&wq->execq);
				goto task_found;
			}

			/* 2. Now the wait queues */
			co_queue_e_t *prev;
			for_each_filter_queue(task, prev, &wq->waitq) {
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
					if (coroutine->parent) { /* If it is a child coroutine, notify parent it terminated */
						coroutine->parent->child = NULL;
						co_routine_flag_set(&coroutine->parent->flags, CO_FLAG_READY);
					}
					co_multi_co_wq_free(wq, task);
					break;
				case CO_RV_YIELD_WAIT:
					co_q_enq(&wq->waitq, task); /* Just move to wait queue */
					break;
				case CO_RV_YIELD_RETURN:
					if (coroutine->parent) /* If it is a child coroutine, notify parent it has intermediate results */
						co_routine_flag_set(&coroutine->parent->flags, CO_FLAG_READY);
					else
						co_q_enq(&wq->execq, task); /* Else reschedule */
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
	co_routine_flag_set(&co->flags, CO_FLAG_READY);
	if (co_atom_cmpxchg(&co->wq->bell.wake_me_up, 1, 0) == 1)
		co_completion_done(&co->wq->bell.bell);
}

#endif /*CO_MULTI_CO_WQ_H*/