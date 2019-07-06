
#ifndef UTILS__CO_QUEUE
#define UTILS__CO_QUEUE

#include "../sys/co_alloc.h"

#define co_queue_define(name, type)                                                                                    \
    struct {                                                                                                           \
        type **q;                                                                                                      \
        int cap;                                                                                                       \
        int sz;                                                                                                        \
        int next;                                                                                                      \
    } name

#define co_queue_init(qu, capacity)                                                                                    \
    ({                                                                                                                 \
        qu.cap = capacity;                                                                                             \
        qu.sz;                                                                                                         \
        qu.next = qu.cap - 1;                                                                                          \
        qu.q = co_mem_alloc(qu.cap * sizeof(qu.q));                                                                    \
        qu.q ? 0 : -1;                                                                                                 \
    })

#define co_queue_destroy(qu) free(qu.q)

#define co_queue_empty(qu) (!qu.sz)

#define co_queue_peek(qu) (qu.q[qu.next])

#define co_queue_enq(qu, e)                                                                                            \
    ({                                                                                                                 \
        if (qu.sz < qu.cap) {                                                                                          \
            ++qu.sz;                                                                                                   \
            if (++qu.next >= qu.cap)                                                                                   \
                qu.next = 0;                                                                                           \
            qu.q[qu.next] = e;                                                                                         \
            0;                                                                                                         \
        } else                                                                                                         \
            -1;                                                                                                        \
    })

#define co_queue_deq(qu)                                                                                               \
    ({                                                                                                                 \
        --qu.sz;                                                                                                       \
        if (--qu.next < 0)                                                                                             \
            qu.next = qu.cap - 1;                                                                                      \
    })

#define co_queue_elem_at(qu, at) qu.q[(qu.next + at) < qu.cap ? (qu.next + at) : (qu.next + at - qu.cap)]

#define co_queue_foreach(qu, var, idx)                                                                                 \
    for (idx = 0, var = co_queue_peek(qu); idx < qu.sz; ++idx, var = co_queue_elem_at(qu, idx))

#endif /*UTILS__CO_QUEUE*/