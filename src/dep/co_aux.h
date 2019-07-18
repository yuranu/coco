#ifndef CO_AUX_H
#define CO_AUX_H
/**
 * @file co_aux.h
 *
 * Contains various system specific utility function required by the lib
 *
 */

#include "co_types.h"
#include <time.h>

#define co_invalid_abstime()                                                                                           \
	(co_abstime_t) { 0 }

#define co_is_invalid_abstime(t) ((*t).tv_sec == 0 && (*t).tv_nsec == 0)

static __inline__ co_errno_t co_get_current_time(co_abstime_t *res) { return clock_gettime(CLOCK_REALTIME, res); }

static __inline__ co_errno_t co_get_time_in_future(co_nanosec_t wait, co_abstime_t *res) {
	co_errno_t rv;
	if ((rv = co_get_current_time(res)) != 0) {
		return rv;
	}
	res->tv_sec += (wait / 1000000000UL);
	res->tv_nsec += (wait % 1000000000UL);
	return 0;
}

static __inline__ co_bool_t co_get_time_ge(const co_abstime_t *t1, const co_abstime_t *t2) {
	return (t1->tv_sec > t2->tv_sec || (t1->tv_sec == t2->tv_sec && t1->tv_nsec >= t2->tv_nsec));
}

static __inline__ co_bool_t co_time_passed(const co_abstime_t *t) {
	co_abstime_t now;
	if (co_is_invalid_abstime(t))
		return 1;
	co_get_current_time(&now);
	return co_get_time_ge(&now, t);
}

#endif /*CO_AUX_H*/