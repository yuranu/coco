#ifndef DEP__CO_ASSERT_H
#define DEP__CO_ASSERT_H

#include <signal.h>
#include <stdio.h>

#define co_assert(what, reason, ...)                                                                                   \
    {                                                                                                                  \
        fprintf(stderr, "Assertion failed at %s:%u:", __FILE__, __LINE__);                                             \
        fprintf(stderr, reason, ##__VA_ARGS__);                                                                        \
        fflush(stderr);                                                                                                \
        raise(SIGABRT);                                                                                                \
    }

#endif /*DEP__CO_ASSERT_H*/