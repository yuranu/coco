#ifndef CO_COROUTINES_H
#define CO_COROUTINES_H
/**
 * @file co_coroutines.h
 *
 * Coroutines definition and yielding macros
 *
 */

#define co_ctx_tname(fname) fname##__co_ctx
#define co_args_tname(fname) fname##__co_args
#define co_locals_tname(fname) fname##__co_locals

#define co_ctx_def(rtype, fname, ...)                                                                                  \
	struct co_args_tname(fname) {                                                                                      \
		__co_pairs(;, ##__VA_ARGS__);                                                                                  \
	};                                                                                                                 \
	struct co_ctx_tname(fname) {                                                                                       \
		co_coroutine_obj_t obj;                                                                                        \
		struct co_args_tname(fname) args;                                                                              \
		struct co_args_tname(fname) * locals_ptr;                                                                      \
		rtype rv;                                                                                                      \
	}

#define co_routine_decl(rtype, fname, ...)                                                                             \
	co_ctx_def(rtype, fname, ##__VA_ARGS__);                                                                           \
	rtype fname(co_coroutine_obj_t *)

#define co_routine_start(fname)                                                                                        \
	__co_state_start:                                                                                                  \
	if (ctx->)

#endif /*CO_COROUTINES_H*/