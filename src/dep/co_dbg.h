#ifndef DEP__CO_ASSERT_H
#define DEP__CO_ASSERT_H

#include "../utils/co_macro.h"
#include <signal.h>
#include <stdio.h>

/*
 * Init debug settings
 */
#ifndef CO_DBG_TRACES
#define CO_DBG_TRACES_
#else
#define CO_DBG_TRACES_ 1
#endif

#ifndef CO_DBG_ASSERTIONS
#define CO_DBG_ASSERTIONS_
#else
#define CO_DBG_ASSERTIONS_ 1
#endif

#define co_assert_(what, reason, ...)                                                                                  \
	if (!(what)) {                                                                                                     \
		fprintf(stderr, "Assertion <%s> failed in %s, %s:%u:", __co_stringify(what), __FUNCTION__, __FILE__,           \
		        __LINE__);                                                                                             \
		fprintf(stderr, reason, ##__VA_ARGS__);                                                                        \
		fflush(stderr);                                                                                                \
		raise(SIGABRT);                                                                                                \
	}

#define co_assert(what, reason, ...) __co_if_empty(CO_DBG_ASSERTIONS_, , co_assert_(what, reason, ##__VA_ARGS__))

/**
 * Do something in debug mode only
 * @param expr Any code snippet
 */
#define co_dbg(expr) __co_if_empty(CO_DBG_TRACES_, , expr)

#define co_dbg_trace(...)                                                                                              \
	co_dbg({                                                                                                           \
		fprintf(stderr, ">>> "__VA_ARGS__);                                                                            \
		fflush(stderr);                                                                                                \
	})

#endif /*DEP__CO_ASSERT_H*/