#ifndef CO_COROUTINE_OBJECT_G
#define CO_COROUTINE_OBJECT_G
/**
 * @file co_coroutine_object.h
 *
 * Aggregates all the functionality related to co_coroutine class
 *
 */

#include "dep/co_dbg.h"
#include "dep/co_queue.h"
#include "dep/co_sync.h"
#include "dep/co_types.h"

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
 * A coroutine can have different exit states.
 *
 * First - RETURN. This means the coroutine returned some intermediate results.
 * Only if exit status is RETURN, it is allowed to check coroutine RV object.
 * After this exit status, it can be re-executed immediately or any other time.
 *
 * Next, BREAK. This means end of coroutine execution. The expected response for
 * this return value is resource deallocation. Trying to re-execute it is
 * unexpected behaviour.
 *
 * Next, WAIT. This means the coroutine waits for external thread to report
 * completion. It comes with conjuction with READY flag. While flag is off,
 * it is prohibited to re-execute this coroutine. It can be completed
 * by ringing the bell on the coroutine, which can be done by any thread,
 * or simply by turning READY flag on from other coroutine context.
 * As soon as READY flag is on, it is allowed to re-execute the coroutine.
 *
 * Next, WAIT_COND. This means coroutine waits for a condition to fulfill,
 * but it does not rely on any flags unlike WAIT. Rather, condition is tested
 * by re-executing the coroutine. After re-execution, it can either return
 * or WAIT_COND again or continue execution to any other state.
 *
 * Last is ERROR. This indicates a critical error, that system cannot recover
 * from. For now it only happens on failure to allocate resources for a
 * task AFTER it already began execution process. Logaically it is similar to
 * stack overflow exception in classic stack model. Will result in SIGABRT
 * if in user space or BUG in kernerl as there is nothing to be done.
 *
 * @todo Implement CO_RV_YIELD_WAIT_COND
 *
 */
typedef enum co_yield_rv {
	/** Terminated cycle, but there are more cycles */
	CO_RV_YIELD_RETURN,
	/** Execution terminated. */
	CO_RV_YIELD_BREAK,
	/** Not terminated, still running asynchronously */
	CO_RV_YIELD_WAIT,
	/** Not terminated, waiting on condition, can be rerun to test condition */
	CO_RV_YIELD_WAIT_COND,
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

void static __inline__ co_multi_co_wq_ring_the_bell(struct co_multi_co_wq *wq);

/**
 * Activate the bell event of coroutine's work queue
 * @param co Coroutine object pointer
 */
void static __inline__ co_coroutine_obj_ring_the_bell(co_coroutine_obj_t *co) {
	co_routine_flag_set(&co->flags, CO_FLAG_READY);
	co_multi_co_wq_ring_the_bell(co->wq);
}

#endif /*CO_COROUTINE_OBJECT_G*/