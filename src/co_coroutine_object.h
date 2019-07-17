#ifndef CO_COROUTINE_OBJECT_G
#define CO_COROUTINE_OBJECT_G
/**
 * @file co_coroutine_object.h
 *
 * Aggregates all the functionality related to co_coroutine class
 *
 */

#include "dep/co_dbg.h"
#include "dep/co_list.h"
#include "dep/co_sync.h"
#include "dep/co_types.h"

struct co_coroutine_obj;
struct co_multi_co_wq;

/**
 * Possible exit code of coroutine
 * A coroutine can have different exit states.
 *
 * @todo Document all the state transitions
 *
 */
typedef enum co_yield_rv {
	/** Terminated cycle, but there are more cycles */
	CO_RV_YIELD_RETURN,
	/** Execution terminated. */
	CO_RV_YIELD_BREAK,
	/** Not terminated, awaiting child coroutine */
	CO_RV_YIELD_AWAIT,
	/** Not terminated, waiting on condition, can be rerun to test condition */
	CO_RV_YIELD_COND_WAIT,
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
	CO_FLAG_SLOW_ALLOC,
	/** Is set on coroutine after it terminated, just before it is destroyed, to notify its callers */
	CO_FLAG_TERM,
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

#define co_routine_flag_set_alloc(flags, alloc) __co_cat_2(co_routine_flag_set_alloc_, alloc)(flags)
#define co_routine_flag_set_alloc_fast(flags) co_routine_flag_clear(flags, CO_FLAG_SLOW_ALLOC)
#define co_routine_flag_set_alloc_slow(flags) co_routine_flag_set(flags, CO_FLAG_SLOW_ALLOC)

/**
 * Generic coroutine object
 */
typedef struct co_coroutine_obj {
	/** Each object is actually a queue element */
	co_list_e_t qe;
	/** Pointes back to work queue. */
	struct co_multi_co_wq *wq;
	/** The pointer to the coroutine function */
	co_yield_rv_t (*func)(struct co_coroutine_obj *);
	/** Resume position inside coroutine function */
	co_ipointer_t ip;
	/** Bitmap with various co_routine flags */
	co_routine_flags_bmp_t flags;
	/** Pointer to all the coroutines awaiting for current coroutine */
	co_list_e_t *await;

	/** Debug only trace function name */
	co_dbg(const char *func_name);
} co_coroutine_obj_t;

void static __inline__ co_multi_co_wq_ring_the_bell(struct co_multi_co_wq *wq);

/**
 * Test whether coroutine is terminated
 * @param co Coroutine object pointer
 * @return 1 if terminated else 0
 */
static __inline__ int co_is_terminated(const co_coroutine_obj_t *co) {
	return co_routine_flag_test(co->flags, CO_FLAG_TERM);
}

/**
 * Activate the bell event of coroutine's work queue
 * @param co Coroutine object pointer
 */
void static __inline__ co_coroutine_obj_ring_the_bell(co_coroutine_obj_t *co) {
	co_routine_flag_set(&co->flags, CO_FLAG_READY);
	co_multi_co_wq_ring_the_bell(co->wq);
}

#endif /*CO_COROUTINE_OBJECT_G*/