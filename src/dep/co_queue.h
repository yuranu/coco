
#ifndef CO_QUEUE
#define CO_QUEUE
/**
 * @file co_queue.h
 */

#include "co_types.h"

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

#define co_q_init()                                                                                                    \
	(co_queue_t) { NULL, NULL }

static __inline__ co_bool_t co_q_empty(co_queue_t *q) { return q->head == NULL; }

static __inline__ void co_q_enq(co_queue_t *q, co_queue_e_t *elem) {
	if (q->tail)
		q->tail->next = elem;
	else
		q->head = elem;
	q->tail = elem;
	q->tail->next = NULL;
}

static __inline__ void co_q_enq_head(co_queue_t *q, co_queue_e_t *elem) {
	if (q->tail == NULL)
		q->tail = elem;
	elem->next = q->head;
	q->head = elem;
}

static __inline__ void co_q_deq(co_queue_t *q) {
	q->head = q->head->next;
	if (!q->head)
		q->tail = NULL;
}

static __inline__ co_queue_e_t *co_q_peek(co_queue_t *q) { return q->head; }

static __inline__ void co_q_enq_q(co_queue_t *dst, co_queue_t *src) {
	if (dst->tail)
		dst->tail->next = src->head;
	else
		dst->head = src->head;
	dst->tail = src->tail;
	*src = co_q_init();
}

#define for_each_co_queue(var, q) for ((var) = (q)->head; (var); (var) = (var)->next)

#define for_each_drain_queue(var, q, peek, deq) for ((var) = peek(q); (var); (var) = peek(q), deq(q))

#endif /*CO_QUEUE*/