
#ifndef UTILS__CO_QUEUE
#define UTILS__CO_QUEUE

#include "../sys/co_alloc.h"

#define co_queue_define(name, type)                                                                                    \
    struct {                                                                                                           \
        type *q;                                                                                                       \
        int cap;                                                                                                       \
        int sz;                                                                                                        \
    } name

#define co_queue_init(qu, capacity)                                                                                    \
    ({                                                                                                                 \
        qu.cap = capacity;                                                                                             \
        qu.sz = 0;                                                                                                     \
        qu.q = co_mem_alloc(qu.cap * sizeof(*qu.q));                                                                   \
        qu.q ? 0 : -1;                                                                                                 \
    })

#endif