#ifndef CO_TIMEOUT_H
#define CO_TIMEOUT_H
/**
 * @file co_timeout.h
 *
 * Contains definition of special coroutine used to handle timeouts
 *
 */

#include "../co_coroutines.h"
#include "co_aux.h"

co_routine_decl(/*void*/, __co_internal_timeout, co_abstime_t, until);

co_yield_rv_t __co_internal_timeout(struct __co_internal_timeout_co_obj *self) {
	co_routine_begin(self, __co_internal_timeout);

	while (!co_time_passed(&self->args.until)) {
		__co_adjust_wake_up(self->obj.wq, &self->args.until);
		co_yield_wait(self);
	}

	co_yield_break();
}

#define co_yield_wait_timeout(self, timeout)                                                                           \
	{                                                                                                                  \
		co_abstime_t __co_until;                                                                                       \
		struct __co_internal_timeout_co_obj *__co_timeout;                                                             \
		co_get_time_in_future(timeout, &__co_until);                                                                   \
		__co_timeout = co_fork_run(self, __co_internal_timeout, __co_until);                                           \
		if (__co_timeout)                                                                                              \
			co_yield_await(self, __co_timeout);                                                                        \
	}

#endif /*CO_TIMEOUT_H*/