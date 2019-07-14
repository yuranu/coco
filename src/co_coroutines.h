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

#define co_ctx_tname(fname) __co_cat_2(fname, ____co_ctx)
#define co_args_tname(fname) __co_cat_2(fname, ____co_args)
#define co_locs_tname(fname) __co_cat_2(fname, ____co_locs)
#define co_rtype_tname(fname) __co_cat_2(fname, ____co_rtype)

/**
 * Retrieve full coroutine ctx pointer from inside coroutine body
 */
#define co_ctx(__co_obj, fname) __co_container_of(__co_obj, struct co_ctx_tname(fname), obj)

/**
 * Naming convention for coroutine sync invocator
 * @todo Implement invocators
 */
#define co_sync_fname(fname) fname

/**
 * Initializer for ctx object of coroutine
 * @param fname Coroutine name
 * @param wq Work queue
 * @param ... Argument initializer list
 */
#define co_routine_ctx_init(fname, wqptr, ...)                                                                         \
	(struct co_ctx_tname(fname)) {                                                                                     \
		.obj.wq = wqptr, .obj.flags = co_routine_flags_init(), .obj.func = co_body_fname(fname),                       \
		.obj.ip = CO_IPOINTER_START, .obj.child = NULL, .obj.parent = NULL, .args = {__co_list_c(__VA_ARGS__)},        \
		.locs_ptr = NULL, co_dbg(.obj.func_name = __co_stringify(fname))                                               \
	}

/**
 * Support macro called from coroutine context to handle critical error.
 * @warning Never use directly unless you know what you are doing
 * @todo Proper handling
 */
#define __co_handle_critical_error() return CO_RV_YIELD_ERROR

/**
 * Prepare child coroutine object for execution
 * Uses fast allocator for memory allocation
 * @param fname Child coroutine to prepare
 * @param ... Optional child arguments
 * @note Only allowed to be called from coroutine context, assumes __co_obj defined.
 */
#define co_prepare_child(fname, ...)                                                                                   \
	({                                                                                                                 \
		struct co_ctx_tname(fname) *__co_child_obj =                                                                   \
		    co_multi_co_wq_alloc_fast(__co_obj->wq, sizeof(struct co_ctx_tname(fname)));                               \
		if (!__co_child_obj) /* ENOMEM */                                                                              \
			__co_handle_critical_error();                                                                              \
		co_routine_flag_clear(&__co_obj->flags, CO_FLAG_SLOW_ALLOC);                                                   \
		*__co_child_obj            = co_routine_ctx_init(fname, __co_obj->wq, ##__VA_ARGS__); /* Pass the args */      \
		__co_child_obj->obj.parent = __co_obj;             /* Link child to parent */                                  \
		__co_obj->child            = &__co_child_obj->obj; /* Link parent to child */                                  \
	})

#define co_routine_invoke(fname, wq, ...)                                                                              \
	({                                                                                                                 \
		co_errno_t __co_rv                   = 0;                                                                      \
		struct co_ctx_tname(fname) *__co_obj = co_multi_co_wq_alloc_slow((wq), sizeof(struct co_ctx_tname(fname)));    \
		if (!__co_obj)                                                                                                 \
			__co_rv = -ENOMEM;                                                                                         \
		co_routine_flag_set(&__co_obj->obj.flags, CO_FLAG_SLOW_ALLOC);                                                 \
		if (!__co_rv) {                                                                                                \
			*__co_obj = co_routine_ctx_init(fname, (wq), ##__VA_ARGS__);                                               \
			__co_rv   = co_multi_src_q_enq(&(wq)->inputq, &__co_obj->obj.qe);                                          \
			if (__co_rv)                                                                                               \
				co_multi_co_wq_free(wq, &__co_obj->obj.qe);                                                            \
		}                                                                                                              \
		__co_rv;                                                                                                       \
	})

/**
 * Naming convention for coroutine body
 */
#define co_body_fname(fname) __co_cat_2(fname, _co_body)

#define co_eval_rtype(rtype) __co_if_empty(rtype, void, rtype)

#define co_set_rv(fname) __co_if_empty(rtype, void, rtype)

#define co_ctx_def(rtype, fname, ...)                                                                                  \
	struct co_args_tname(fname) { /* Define args type */                                                               \
		__co_pairs(;, ##__VA_ARGS__);                                                                                  \
	};                                                                                                                 \
	struct co_ctx_tname(fname) { /* Define full ctx object */                                                          \
		co_coroutine_obj_t obj;                                                                                        \
		struct co_args_tname(fname) args;                                                                              \
		struct co_locs_tname(fname) * locs_ptr;                                                                        \
		__co_if_empty(rtype, , rtype rv);                                                                              \
	}

/**
 * Co routine declaration, must come before body
 */
#define co_routine_decl(rtype, fname, ...)                                                                             \
	co_ctx_def(rtype, fname, ##__VA_ARGS__);                 /* Define ctx */                                          \
	co_yield_rv_t co_body_fname(fname)(co_coroutine_obj_t *) /* Declare function */

/**
 * Co-routine body prototype
 */
#define co_routine_body(fname) co_yield_rv_t co_body_fname(fname)(co_coroutine_obj_t * __co_obj)

/**
 * Start co routine code.
 * Always must be the first statement inside co-routine body.
 * @param fname Coroutine name
 * @param co_obj Coroutine object funciton parameter name
 * @param ... List of local variables of form [type, name, type, name, ...]
 * @note Only allowed to be called from coroutine context, assumes __co_obj defined.
 * @todo Implement locals
 */
#define co_routine_start(fname, ...)                                                                                   \
	/** @todo Add code that defines local object struct */                                                             \
	struct co_ctx_tname(fname) *__co_ctx = co_ctx(__co_obj, fname);                                                    \
	struct co_args_tname(fname) *args    = &__co_ctx->args;                                                            \
	__co_if_empty(__VA_ARGS__, , struct co_locs_tname(fname) *locs = __co_ctx->locs_ptr);                              \
	/** @todo Add code that initializes locals object */                                                               \
	__co_label_start:                                                                                                  \
	/* Coroutine always assumes results not ready, and tries to prove this wrong */                                    \
	co_routine_flag_clear(&__co_obj->flags, CO_FLAG_READY);                                                            \
	if (__co_obj->ip != CO_IPOINTER_START)                                                                             \
		goto *(&&__co_label_start + __co_obj->ip);

#define co_gen_label(labael, n) __co_cat_2(labael, n)
#define co_label_checkpoint co_gen_label(__co_label_checkpoint_, __LINE__)
#define co_label_checkpoint_2 co_gen_label(__co_label_checkpoint_2_, __LINE__)

/**
 * Terminate coroutine execution
 */
#define co_yield_break() return CO_RV_YIELD_BREAK

/**
 * Yield coroutine, returning a value
 * @param ... Optional return value
 * @note Only allowed to be called from coroutine context, assumes __co_obj defined.
 */
#define co_yield_return(...)                                                                                           \
	__co_if_empty(__VA_ARGS__, , __co_ctx->rv = __VA_ARGS__);  /* Set rv */                                            \
	__co_obj->ip = &&co_label_checkpoint - &&__co_label_start; /* Save return point */                                 \
	return CO_RV_YIELD_RETURN;                                                                                         \
	co_label_checkpoint:                                                                                               \
	__co_nop() /* Next execution will start from here */

/**
 * Yield coroutine and send it to sleep
 * @param ... Optional timeout
 * @todo Implement timeout
 * @note Only allowed to be called from coroutine context, assumes __co_obj defined.
 */
#define co_yield_wait(...)                                                                                             \
	__co_obj->ip = &&co_label_checkpoint - &&__co_label_start; /* Save return point */                                 \
	return CO_RV_YIELD_WAIT;                                                                                           \
	co_label_checkpoint:                                                                                               \
	__co_nop() /* Next execution will start from here */

/**
 * Invoke coroutine and iterate over its results
 * @param fname Coroutine to invoke
 * @param var Variable to store child rv to. Can be left empty to discard rv.
 * @param args Optional child invocation args comma separated tuple (a,b,c,...)
 * @param code Inner loop code snippet enclosed in {}
 * @note Only allowed to be called from coroutine context, assumes __co_obj defined.
 */
#define co_foreach_yield(var, fname, args, code)                                                                       \
	co_prepare_child(fname, __co_list_c args);                                                                         \
	__co_obj->ip = &&co_label_checkpoint - &&__co_label_start; /* Prepare loop */                                      \
	co_q_enq(&__co_obj->wq->execq, &__co_obj->child->qe);      /* Enqueue the child, we can use the execq for that */  \
	return CO_RV_YIELD_WAIT;                                   /* And now go to sleep, child will wake us up */        \
	co_label_checkpoint:                                       /* We are here after child response */                  \
	if (__co_obj->child) {                                     /* Child not terminated */                              \
		__co_if_empty(                                                                                                 \
		    var, ,                                                                                                     \
		    var = __co_container_of(__co_obj->child, struct co_ctx_tname(fname), obj)->rv); /* Grab the results */     \
		code;                                                                               /* Invoke user code */     \
		/* Next iteration */                                                                                           \
		__co_obj->ip = &&co_label_checkpoint - &&__co_label_start; /* Fix loop if damaged by user code */              \
		co_q_enq(&__co_obj->wq->execq, &__co_obj->child->qe);                                                          \
		return CO_RV_YIELD_WAIT;                                                                                       \
	}

/**
 * Invoke coroutine and return its results
 * Special case of co_foreach_yield, which simply returns child's results
 * @param fname Coroutine to invoke
 * @param ... Optional child invocation args
 * @note Only allowed to be called from coroutine context, assumes __co_obj defined.
 */
#define co_foreach_yield_return(fname, ...)                                                                            \
	co_prepare_child(fname, ##__VA_ARGS__);                                                                            \
	co_label_checkpoint_2:                                                                                             \
	__co_obj->ip = &&co_label_checkpoint - &&__co_label_start; /* Prepare loop */                                      \
	co_q_enq(&__co_obj->wq->execq, &__co_obj->child->qe);      /* Enqueue the child, we can use the execq for that */  \
	return CO_RV_YIELD_WAIT;                                   /* And now go to sleep, child will wake us up */        \
	co_label_checkpoint:                                       /* We are here after child response */                  \
	if (__co_obj->child) {                                     /* Child not terminated */                              \
		__co_ctx->rv = __co_container_of(__co_obj->child, struct co_ctx_tname(fname), obj)->rv; /* Grab the results */ \
		/* Next iteration */                                                                                           \
		__co_obj->ip = &&co_label_checkpoint_2 - &&__co_label_start; /* Fix loop to start from child submission */     \
		return CO_RV_YIELD_RETURN;                                                                                     \
	}

#endif /*CO_COROUTINES_H*/