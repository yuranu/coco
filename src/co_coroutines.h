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
 * TODO: Implement invocators
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
		.obj.wq = wqptr, .obj.ready = 0, .obj.func = co_body_fname(fname), .obj.ip = CO_IPOINTER_START,                \
		.obj.parent = NULL, .args = {__co_list_c(__VA_ARGS__)}, .locs_ptr = NULL                                       \
	}

#define co_routine_invoke(fname, wq, ...)                                                                              \
	({                                                                                                                 \
		co_errno_t __co_rv = 0;                                                                                        \
		struct co_ctx_tname(fname) *__co_obj =                                                                         \
			(wq)->slowq.alloc->alloc((wq)->slowq.alloc, sizeof(struct co_ctx_tname(fname)));                           \
		if (!__co_obj)                                                                                                 \
			__co_rv = -ENOMEM;                                                                                         \
		if (!__co_rv) {                                                                                                \
			*__co_obj = co_routine_ctx_init(fname, (wq), ##__VA_ARGS__);                                               \
			__co_rv = co_multi_src_q_enq(&(wq)->inputq, &__co_obj->obj.qe);                                     \
			if (__co_rv)                                                                                               \
				(wq)->slowq.alloc->free((wq)->slowq.alloc, __co_obj);                                                  \
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
	co_ctx_def(rtype, fname, ##__VA_ARGS__);				 /* Define ctx */                                          \
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
 */
#define co_routine_start(fname, ...)                                                                                   \
	/* TODO: Add code that defines local object struct */                                                              \
	struct co_ctx_tname(fname) *__co_ctx = co_ctx(__co_obj, fname);                                                    \
	struct co_args_tname(fname) *args = &__co_ctx->args;                                                               \
	__co_if_empty(__VA_ARGS__, , struct co_locs_tname(fname) *locs = __co_ctx->locs_ptr);                              \
	/* TODO: Add code that initializes locals object */                                                                \
	__co_label_start:                                                                                                  \
	if (__co_obj->ip != CO_IPOINTER_START)                                                                             \
		goto *(&&__co_label_start + __co_obj->ip);

#define co_label_checkpoint_n(n) __co_cat_2(__co_label_checkpoint_, n)
#define co_label_checkpoint co_label_checkpoint_n(__LINE__)

/**
 * Terminate coroutine execution
 */
#define co_yield_break() return CO_RV_YIELD_BREAK

/**
 * Yield coroutine, returning a value
 * @param ... Optional return value
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
 * TODO: Implement timeout
 */
#define co_yield_wait(...)                                                                                             \
	__co_obj->ip = &&co_label_checkpoint - &&__co_label_start; /* Save return point */                                 \
	return CO_RV_YIELD_WAIT;                                                                                           \
	co_label_checkpoint:                                                                                               \
	__co_nop() /* Next execution will start from here */

#endif /*CO_COROUTINES_H*/