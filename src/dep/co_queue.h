
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
	q->tail->next = elem;
	q->tail = elem;
}

static __inline__ void co_q_enq_head(co_queue_t *q, co_queue_e_t *elem) {
	if (q->tail == NULL)
		q->tail = elem;
	elem->next = q->head;
	q->head = elem;
}

static __inline__ void co_q_deq(co_queue_t *q) { q->head = q->head->next; }

static __inline__ co_queue_e_t *co_queue_peek(co_queue_t *q) { return q->head; }

static __inline__ void co_q_enq_q(co_queue_t *dst, co_queue_t *src) {
	dst->tail->next = src->head;
	dst->tail = src->tail;
	*src = co_q_init();
}

#define co_queue_for_each(var, q) for ((var) = (q)->head; (var) != (q)->tail->next; (var) = (var)->next)

#endif /*CO_QUEUE*/