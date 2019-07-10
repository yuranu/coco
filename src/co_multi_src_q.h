#ifndef CO_MULTI_SRC_Q_H
#define CO_MULTI_SRC_Q_H

/**
 * Multi source queue
 * ==================
 * The aim of this module is to provide a queue that can receive input
 * from multiple threads (enque) and provide output to a single thread (deque).
 *
 * We rely on atomic instruction xchg for implementation. There is an assumption that
 * the load is low enough to allow smooth lockless operation of the queu in good path,
 * with rare collision handling. The aim is to achieve good *average* runtime.
 *
 *
 * !!! IMPORTANT: Memory allocation is an issue and may become a bottleneck.
 * In context of this module, it is just assumed memory allocation *magically* works.
 * How it is done is left to an allocator to solve.
 *
 * The idea:
 *    For enqueue, maintain N ( ~ number of CPUs ) separate queues, each guarded by xchg flag.
 *    Guard is similar to spinlock, but we do not spin on one queue more than once.
 *    If we failed to aquire a lock, we immediately move to the next queue.
 *    The assumption is that we will eventually lock some queue very quickly.
 *
 *    For dequeue, maintain one unified queue. On dequeue, first drain the unified queue.
 *    If it is empty, check all the other queues. For each, drain it and put into the unified queue.
 *
 */

#include "dep/co_atomics.h"
#include "dep/co_types.h"
#include "utils/co_macro.h"

struct co_queu_e;
/**
 * Queue element
 */
typedef struct co_queu_e {
	struct co_queu_e *next;
} co_queue_e_t;

/**
 * Queue
 */
typedef struct co_queu {
	struct co_queu_e *head, *tail;
} co_queue_t;

#define co_queue_init()                                                                                                \
	(co_queue_t) { NULL, NULL }

static __inline__ co_bool_t co_q_empty(co_queue_t *q) { return q->head == NULL; }

static __inline__ void co_q_enq(co_queue_t *q, co_queue_e_t *elem) {
	q->tail->next = elem;
	q->tail = elem;
}

static __inline__ void co_q_deq(co_queue_t *q) { q->head = q->head->next; }

static __inline__ co_queue_e_t *co_queue_peek(co_queue_t *q) { return q->head; }

static __inline__ void co_q_enq_q(co_queue_t *dst, co_queue_t *src) {
	dst->tail->next = src->head;
	dst->tail = src->tail;
	*src = co_queue_init();
}

#define co_queue_for_each(var, q) for ((var) = (q)->head; (var) != (q)->tail->next; (var) = (var)->next)

#if defined(CO_MULTI_SRC_Q_N) && CO_MULTI_SRC_Q_N > 0
#define CO_MULTI_SRC_STATIC_SIZED
#endif

typedef struct co_multi_src_q {
#ifdef CO_MULTI_SRC_STATIC_SIZED
	co_atom_arr_decl(CO_MULTI_SRC_Q_N) locks; /* Locks */
	co_queue_t[CO_MULTI_SRC_Q_N] iqs;		  /* Input queueues */
#else
	co_atom_t *locks; /* Locks */
	co_queue_t *iqs;  /* Input queueues */
	co_size_t sz;
#endif
	co_queue_t mq; /* Main queue */
	co_size_t iqi; /* Last input queue we read from */
} co_multi_src_q_t;

#ifndef CO_MULTI_SRC_STATIC_SIZED
#define co_multi_src_q_sz(q) ((q)->sz)
#else
#define co_multi_src_q_sz(q) (CO_MULTI_SRC_Q_N)
#endif

/**
 * Initialize multi source queue
 * @param q Output queue
 * @param size Number of iqs to allocate. Ignored if static sized.
 * @return 0 or errno
 */
co_errno_t co_multi_src_q_init(co_multi_src_q_t *q, co_size_t size) {
#ifndef CO_MULTI_SRC_STATIC_SIZED
	if ((q->locks = co_atom_alloc(size)) == NULL)
		return -ENOMEM;
	if ((q->iqs = co_malloc(size * sizeof(co_queue_t))) == NULL) {
		co_atom_free(q->locks);
		return -ENOMEM;
	}
	q->sz = size;
#else
	(void)size;
#endif
	{
		int i;
		for (i = 0; i < co_multi_src_q_sz(q); ++i) {
			q->locks[i] = co_atom_init(0);
			q->iqs[i] = co_queue_init();
		}
		q->mq = co_queue_init();
		q->iqi = 0;
	}
	return 0;
}

/**
 * Get the first element of queue without dequeueing it
 * @param q Multi source queue pointer
 * @return Multi queue element or NULL if empty
 */
static __inline__ co_queue_e_t *co_multi_src_q_peek(co_multi_src_q_t *q) {
	int i;
	if (!co_q_empty(&q->mq)) /* First always check mq */
		return co_queue_peek(&q->mq);
	for (i = 0; i < co_multi_src_q_sz(q); ++i) { /* If nothing in mq, check all iqs */
		if (!co_q_empty(&q->iqs[q->iqi]) && !co_atom_xchg(&q->locks[q->iqi], 1)) {
			/* Iq is non empty and lockable - put all its contets to mq*/
			co_q_enq_q(&q->mq, &q->iqs[q->iqi]);
			/* Unlock iq */
			co_atom_xchg_unlock(&q->locks[q->iqi]);
			return co_multi_src_q_peek(q); /* Now we have something in mq for sure */
		}
	}
	return NULL;
}

/**
 * Remove first element from queue
 * Assumes queue is not empty. Check it before calling.
 * @param q Multi source queue pointer
 */
static __inline__ void co_multi_src_q_deq(co_multi_src_q_t *q) {
	/* No much logic - delegate work to mq */
	return co_q_deq(&q->mq);
}

#endif /*CO_MULTI_SRC_WQ_H*/