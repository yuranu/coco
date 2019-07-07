
#ifndef UTILS__CO_QUEUE
#define UTILS__CO_QUEUE

#include "../sys/co_alloc.h"

#define co_queue_define(name, type)                                                                                    \
    struct {                                                                                                           \
        type *q;                                                                                                       \
        int cap;                                                                                                       \
        int head;                                                                                                      \
        int tail;                                                                                                      \
    } name

#define co_queue_init(qu, capacity)                                                                                    \
    ({                                                                                                                 \
        qu.cap = capacity;                                                                                             \
        qu.head = qu.tail = 0;                                                                                         \
        qu.q = co_mem_alloc(qu.cap * sizeof(qu.q));                                                                    \
        qu.q ? 0 : -ENOMEM;                                                                                            \
    })

#define co_queue_destroy(qu) free(qu.q)

#define co_queue_empty(qu) (qu.head == qu.tail)

#define co_queue_peek(qu) (qu.q[qu.head])

#define co_queue_enq(qu, e)                                                                                            \
    ({                                                                                                                 \
        int __err_ = 0;                                                                                                \
        int __newtail_ = qu.tail + 1;                                                                                  \
        if (__newtail_ >= qu.cap)                                                                                      \
            __newtail_ = 0;                                                                                            \
        if (__newtail_ != qu.head) {                                                                                   \
            qu.q[qu.tail] = e;                                                                                         \
            qu.tail = __newtail_;                                                                                      \
        } else {                                                                                                       \
            --qu.tail;                                                                                                 \
            __err_ = -ENOMEM;                                                                                          \
        }                                                                                                              \
        __err_;                                                                                                        \
    })

/* Warning: no validaiton that queue is not empty */
#define co_queue_deq(qu)                                                                                               \
    ({                                                                                                                 \
        ++qu.head;                                                                                                     \
        if (qu.head >= qu.cap)                                                                                       \
            qu.head = 0;                                                                                               \
    })

#define co_queue_elem_at(qu, at) qu.q[(qu.next + at) < qu.cap ? (qu.next + at) : (qu.next + at - qu.cap)]

#define co_queue_foreach(qu, var, idx)                                                                                 \
    for (idx = 0, var = co_queue_peek(qu); idx < qu.sz; ++idx, var = co_queue_elem_at(qu, idx))

#endif /*UTILS__CO_QUEUE*/