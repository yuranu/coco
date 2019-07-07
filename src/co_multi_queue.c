#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>

#include "utils/co_queue.h"

typedef pthread_t co_tid_t;
typedef unsigned char co_hash_t;

typedef enum co_routine_rv {
    CO_RV_TERM /* I am done, no rv */,
    CO_RV_MORE /* I have an rv, and I am not done */,
    CO_RV_WAIT /* I don't have an rv right now, bit I am not done */
} co_routine_rv_t;

typedef co_routine_rv_t (*co_routine_t)(void *);

#define CO_TID_FREE 0  /* Free cell */
#define CO_TID_READY 1 /* Cell written, ready to be read */

/**
 * Retrieve a unique thask id / thread id. Must be not 0 or 1.
 * TODO: Currently I am not not sure pthread_self cannot be 0 or 1.
 *       Implement some infrastructure to ensure it.
 * @return Task ID
 */
#define co_get_tid()                                                                                                   \
    ({                                                                                                                 \
        assert(pthread_self() > 1);                                                                                    \
        pthread_self();                                                                                                \
    })

/**
 * Atomics on task id type
 */
#define co_declare_atm_bool(name) int name __attribute__((aligned(32)))
#define co_declare_atm_tid(name) co_tid_t name __attribute__((aligned(64)))
#define co_atm_cmpxchg(ptr, old, new) __sync_val_compare_and_swap(ptr, old, new)
#define co_atm_read(ptr) (*ptr)
#define co_atm_set(ptr, val)                                                                                           \
    {                                                                                                                  \
        *ptr = val;                                                                                                    \
        __sync_synchronize();                                                                                          \
    }

#define CO_HASH_BITS 4
#define CO_HASH_MAX (1UL << CO_HASH_BITS)
#define CO_HASH_MASK (CO_HASH_MAX - 1UL)
#define CO_HASH_RETRY CO_HASH_MAX

/* Simple Knuth multiplication for now */
#define co_hash(key) (((2654435769UL * key) >> 32) & CO_HASH_MASK)

typedef struct co_routine_task {
    co_routine_t func;          /* Pointer to handler function */
    void *param;                /* Pointer to function params */
    co_declare_atm_bool(ready); /* Set when rv is ready after exiting with CO_RV_WAIT */
} co_routine_task_t;

typedef struct co_work_sync_object {
    co_declare_atm_tid(req);
    co_routine_task_t *task;
} co_work_sync_object_t;

typedef struct co_multi_queue {
    co_work_sync_object_t syncs[CO_HASH_MAX];
    co_queue_define(ex_q, co_routine_task_t *);
} co_multi_queue_t;

int co_multi_enq(co_multi_queue_t *mq, co_routine_t func, void *param) {
    int round;
    const co_tid_t tid = co_get_tid();
    co_hash_t hash = co_hash(tid);
    co_routine_task_t *task = co_mem_alloc(sizeof(*task));
    if (!task)
        return -ENOMEM;
    *task = (co_routine_task_t){.func = func, .param = param};
    co_atm_set(&task->ready, 0);

    for (round = 0; round < CO_HASH_RETRY; ++round) {

        /* Try to lock a cell */
        if (co_atm_cmpxchg(&mq->syncs[hash].req, CO_TID_FREE, tid) == CO_TID_FREE) {
            /* Success */
            mq->syncs[hash].task = task;
            co_atm_set(&mq->syncs[hash].req, CO_TID_READY);
            return 0;
        }

        if (++hash >= CO_HASH_RETRY)
            hash = 0;
    }

    return -EAGAIN;
}

int co_mq_loop(co_multi_queue_t *mq) {
    int hash = 0;
    do {

        if (!co_queue_empty(mq->ex_q)) {
            co_routine_task_t *task = co_queue_peek(mq->ex_q);
            co_routine_rv_t rv = task->func(task->param);
            if (rv == CO_RV_TERM) {
                co_queue_deq(mq->ex_q);
                co_mem_free(task); /* TBD: Who should free the memory? */
            }
            continue;
        }

        /* Harvest new tasks */
        if (mq->syncs[hash].req == CO_TID_READY) {
            if (!co_queue_enq(mq->ex_q, mq->syncs[hash].task))
                co_atm_set(&mq->syncs[hash].req, CO_TID_FREE);
            continue;
        }
        if (++hash >= CO_HASH_RETRY)
            hash = 0;

    } while (1);
}

co_routine_rv_t sample_task(void *param) {
    int *pi = (int *)param;
    printf("%d-%ld\n", *pi, co_get_tid());
    ++(*pi);
    if (!(*pi % 5))
        return CO_RV_TERM;
    else
        return CO_RV_MORE;
}

void *sample_worker(void *param) {
    co_multi_queue_t *mq = (co_multi_queue_t *)param;
    int i, *pi = malloc(sizeof(int));
    for (i = 1; i < 20; i += 10) {
        pi = malloc(sizeof(int));
        *pi = co_get_tid() + i;
        if (co_multi_enq(mq, sample_task, (void *)pi) < 0) {
            printf("%ld FaILED\n", co_get_tid());
        }
    }
    free(pi);
    return NULL;
}

int main() {
    int i;
    pthread_t thread[10];
    co_multi_queue_t mq;
    co_queue_init(mq.ex_q, 100);
    for (i = 0; i < CO_HASH_MAX; ++i) {
        co_atm_set(&mq.syncs[i].req, CO_TID_FREE);
    }
    for (i = 0; i < 10; ++i) {
        pthread_create(&thread[i], 0, sample_worker, &mq);
    }
    for (i = 0; i < 10; ++i) {
        pthread_join(thread[i], NULL);
    }
    co_mq_loop(&mq);
    return 0;
}