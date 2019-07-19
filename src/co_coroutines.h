#ifndef CO_COROUTINES_H
#define CO_COROUTINES_H
/**
 * @file co_coroutines.h
 *
 * Coroutines definition and yielding macros
 *
 */

#include "co_multi_co_wq.h"
#include "utils/co_macro.h"

/*
 * Naming conventions Section
 */
#define co_ctx_tname(fname) __co_cat_2(fname, _co_obj)
#define co_args_tname(fname) __co_cat_2(fname, _co_args)
#define co_locs_tname(fname) __co_cat_2(fname, _co_locs)
#define co_rtype_tname(fname) __co_cat_2(fname, _co_rtype)
#define co_wrapper_fname(fname) __co_cat_2(fname, _co_wrapper)
#define co_body_fname(fname) fname
#define co_sync_fname(fname) __co_cat_2(fname, _co_sync)

/*
 * Labels section
 */
#define co_gen_label(labael, n) __co_cat_2(labael, n)
#define co_label_checkpoint co_gen_label(__co_label_checkpoint_, __LINE__)
#define co_label_checkpoint_await co_gen_label(__co_label_checkpoint_await_, __LINE__)
#define co_label_checkpoint_1 co_gen_label(__co_label_checkpoint_1_, __LINE__)
#define co_label_checkpoint_2 co_gen_label(__co_label_checkpoint_2_, __LINE__)

/**
 * Retrieve full coroutine ctx pointer from inside coroutine body
 * @param self Coroutine self pointer
 * @param fname Coroutine name
 */
#define co_ctx(self, fname) __co_container_of(self, struct co_ctx_tname(fname), obj)

/**
 * Extract rv from coroutine object
 * @param target Coroutine self pointer
 * @param fname Coroutine name
 */
#define co_get_rv(target, fname) (co_assert co_ctx(self, fname)->rv)

/**
 * Initializer for ctx object of coroutine
 * @param fname Coroutine name
 * @param wq Work queue
 * @param ... Argument initializer list
 */
#define co_routine_ctx_init(fname, wqptr, ...)                                                                         \
	(struct co_ctx_tname(fname)) {                                                                                     \
		.obj.wq = wqptr, .obj.flags = co_routine_flags_init(), .obj.func = co_wrapper_fname(fname),                    \
		.obj.ip = CO_IPOINTER_START, .obj.await = NULL, .args = {__VA_ARGS__}, .locs = NULL,                           \
		co_dbg(.obj.func_name = __co_stringify(fname))                                                                 \
	}

/**
 * Create and initialize coroutine obj
 * @param wq Corotine work queue
 * @param alloc Fast or slow allocator
 * @param fname Coroutine name
 * @param ... Coroutine arguments
 * @note Internal
 */
#define __co_new(wq, alloc, fname, ...)                                                                                \
	({                                                                                                                 \
		struct co_ctx_tname(fname) *__co_new_obj =                                                                     \
			co_multi_co_wq_alloc(wq, alloc, sizeof(struct co_ctx_tname(fname)));                                       \
		if (__co_new_obj) {                                                                                            \
			co_routine_flag_set_alloc(&__co_new_obj->obj.flags, alloc);                                                \
			*__co_new_obj = co_routine_ctx_init(fname, wq, ##__VA_ARGS__);                                             \
		}                                                                                                              \
		__co_new_obj;                                                                                                  \
	})

/**
 * Create and initialize coroutine obj from other coroutine context
 * @param self Calling coroutine
 * @param fname Target coroutine name
 * @param ... Coroutine arguments
 */
#define co_fork(self, fname, ...) __co_new((self)->obj.wq, fast, fname, ##__VA_ARGS__)

/**
 * Create and initialize coroutine obj from other coroutine context, then run it
 * @param self Calling coroutine
 * @param fname Target coroutine name
 * @param ... Coroutine arguments
 */
#define co_fork_run(self, fname, ...)                                                                                  \
	({                                                                                                                 \
		struct co_ctx_tname(fname) *__co_fork = co_fork(self, fname, ##__VA_ARGS__);                                   \
		if (__co_fork)                                                                                                 \
			co_run(self, __co_fork);                                                                                   \
		__co_fork;                                                                                                     \
	})

/**
 * Create and initialize coroutine obj from outside of the coroutine context
 * @param wq Coroutine work queue to schedule on
 * @param fname Target coroutine name
 * @param ... Coroutine arguments
 */
#define co_new(wq, fname, ...) __co_new(wq, slow, fname, ##__VA_ARGS__)

/**
 * Schedule coroutine to start or continue running
 * @param self Coroutine self pointer
 * @param target Coroutine object pointer to run
 */
#define co_run(self, target)                                                                                           \
	({                                                                                                                 \
		co_assert((self) && (target) && (self)->obj.wq == (target)->obj.wq);                                           \
		co_q_enq(&(self)->obj.wq->execq, &(target)->obj.qe);                                                           \
		target;                                                                                                        \
	})

/**
 * Extract coroutine out of execution queue
 * @param self Coroutine self pointer
 * @param target Coroutine object pointer to pause
 */
#define co_pause(self, target)                                                                                         \
	co_assert((self)->obj.wq == (target)->obj.wq);                                                                     \
	co_q_cherry_pick(&(self)->obj.wq->execq, &(target)->obj.qe)

/**
 * Schedule coroutine to start, from external context
 * @param wq Coroutine routine work queue pointer
 * @param target Coroutine object pointer to run
 */
#define co_schedule(_wq, target)                                                                                       \
	co_assert(_wq == (target)->obj.wq);                                                                                \
	co_multi_src_q_enq(&(_wq)->inputq, &(target)->obj.qe);                                                             \
	co_multi_co_wq_ring_the_bell(_wq)

/**
 * Terminate coroutine execution
 */
#define co_yield_break() return CO_RV_YIELD_BREAK

/**
 * Await other coroutine
 * @param self Coroutine self pointer
 * @param target Coroutine object pointer to await
 */
#define co_yield_await(self, target)                                                                                   \
	(self)->obj.ip      = &&co_label_checkpoint_await - &&__co_label_start;                                            \
	(self)->obj.qe.next = (target)->obj.await;                                                                         \
	(target)->obj.await = &(self)->obj.qe;                                                                             \
	return CO_RV_YIELD_AWAIT;                                                                                          \
co_label_checkpoint_await:                                                                                             \
	__co_nop();

/**
 * Await for next response of previously invoked coroutine
 * @param self Coroutine self pointer
 * @param target Coroutine object pointer to await
 * @deprecated
 */
#define co_yield_await_next(self, target)                                                                              \
	(self)->obj.qe.next = (target)->obj.await;                                                                         \
	(target)->obj.await = &(self)->obj.qe;                                                                             \
	return CO_RV_YIELD_AWAIT

/**
 * Awaits on target and executes following code block it target yielded and still alive
 * @param self Calling coroutine
 * @param target Coroutine object
 */
#define if_co_yield_await(self, target)                                                                                \
	co_yield_await(self, target);                                                                                      \
	if (!co_routine_flag_test((target)->obj.flags, CO_FLAG_TERM))

/**
 * Loops while target await returns results
 * @param self Calling coroutine
 * @param target Coroutine object
 * @note Damn, this macro is my masterpiece !!!
 */
#define while_co_yield_await(self, target)                                                                             \
	while (({                                                                                                          \
		(self)->obj.ip      = &&co_label_checkpoint_await - &&__co_label_start;                                        \
		(self)->obj.qe.next = (target)->obj.await;                                                                     \
		(target)->obj.await = &(self)->obj.qe;                                                                         \
		if (!co_routine_flag_test((target)->obj.flags, CO_FLAG_TERM))                                                  \
			return CO_RV_YIELD_AWAIT;                                                                                  \
		0;                                                                                                             \
	}))                                                                                                                \
	co_label_checkpoint_await:                                                                                         \
		if (!co_routine_flag_test((target)->obj.flags, CO_FLAG_TERM))

/**
 * Awaits on target and executes following code block it target yielded and still alive
 * @param self Calling coroutine
 * @param target Coroutine object
 * @note Require both self a target same type of non-void rv
 */
#define for_each_yield_return(self, target)                                                                            \
	while_co_yield_await(self, target) {                                                                               \
		co_pause(self, target);                                                                                        \
		co_yield_return(self, (target)->rv);                                                                           \
		co_run(self, target);                                                                                          \
	}

/**
 * Yield coroutine, returning a value
 * @param self Calling coroutine
 * @param ... Optional return value
 */
#define co_yield_return(self, ...)                                                                                     \
	__co_if_empty(__VA_ARGS__, , (self)->rv = __VA_ARGS__);      /* Set rv */                                          \
	(self)->obj.ip = &&co_label_checkpoint - &&__co_label_start; /* Save return point */                               \
	return CO_RV_YIELD_RETURN;                                                                                         \
co_label_checkpoint:                                                                                                   \
	__co_nop() /* Next execution will start from here */

/**
 * Simply yield and wait until called again if ever
 * @param self Calling coroutine
 * @param cond Condition to wait for
 */
#define co_yield_wait(self)                                                                                            \
	(self)->obj.ip = &&co_label_checkpoint - &&__co_label_start; /* Save return point */                               \
	return CO_RV_YIELD_COND_WAIT;                                                                                      \
co_label_checkpoint:

/**
 * Yield coroutine and send it to sleep until a condition is fulfilled
 * @param self Calling coroutine
 * @param cond Condition to wait for
 */
#define co_yield_wait_cond(self, cond)                                                                                 \
	co_yield_wait(self);                                                                                               \
	if (!(cond))                                                                                                       \
		return CO_RV_YIELD_COND_WAIT;

/**
 * Yield coroutine and send it to sleep indefinitely until external wake up event
 * @param self Calling coroutine
 * @param ... Optional return value
 */
#define co_yield_wait_ready(self)                                                                                      \
	{                                                                                                                  \
		co_routine_flag_clear(&(self)->obj.flags, CO_FLAG_READY);                                                      \
		co_yield_wait_cond(self, co_routine_flag_test((self)->obj.flags, CO_FLAG_READY))                               \
	}

#define co_ctx_def(rtype, fname, ...)                                                                                  \
	struct co_args_tname(fname) { /* Define args type */                                                               \
		__co_pairs(;, ##__VA_ARGS__);                                                                                  \
	};                                                                                                                 \
	struct co_ctx_tname(fname) { /* Define full ctx object */                                                          \
		co_coroutine_obj_t obj;                                                                                        \
		struct co_args_tname(fname) args;                                                                              \
		struct co_locs_tname(fname) * locs;                                                                            \
		__co_if_empty(rtype, , rtype rv);                                                                              \
	}

/**
 * Co routine declaration, must come before body
 */
#define co_routine_decl(rtype, fname, ...)                                                                             \
	co_ctx_def(rtype, fname, ##__VA_ARGS__); /* Define ctx */                                                          \
	extern co_routine_body_proto(fname);     /* Declare body function */                                               \
	static __inline__ co_yield_rv_t co_wrapper_fname(fname)(co_coroutine_obj_t * self) {                               \
		return co_body_fname(fname)(co_ctx(self, fname));                                                              \
	}

/**
 * Co-routine body prototype
 */
#define co_routine_body_proto(fname) co_yield_rv_t co_body_fname(fname)(struct co_ctx_tname(fname) * self)

/**
 * Start co routine code section.
 * Always must be the first statement inside co-routine body.
 * @param self Coroutine self pointer
 * @param fname Coroutine name
 */
#define co_routine_begin(self, fname)                                                                                  \
__co_label_start:                                                                                                      \
	/* Coroutine always assumes results not ready, and tries to prove this wrong */                                    \
	if ((self)->obj.ip != CO_IPOINTER_START)                                                                           \
		goto *(&&__co_label_start + (self)->obj.ip);

#endif /*CO_COROUTINES_H*/