
#ifndef CO_LIST_H
#define CO_LIST_H
/**
 * @file co_list.h
 */

#include "co_dbg.h"
#include "co_types.h"

struct co_list_e;
/**
 * Linked list element
 */
typedef struct co_list_e {
	struct co_list_e *next;
} co_list_e_t;

/**
 * Queue, based on linked list
 */
typedef struct co_queue {
	struct co_list_e *head, *tail;
	co_size_t count;
} co_queue_t;

#define co_q_init()                                                                                                    \
	(co_queue_t) { NULL, NULL, 0 }

static __inline__ co_bool_t co_q_empty(co_queue_t *q) { return q->head == NULL; }

static __inline__ co_size_t co_q_len(co_queue_t *q) {
	int i            = 0;
	co_list_e_t *ptr = q->head;
	while (ptr) {
		++i;
		ptr = ptr->next;
	}
	return i;
}

static __inline__ void co_q_enq(co_queue_t *q, co_list_e_t *elem) {
	if (q->tail)
		q->tail->next = elem;
	else
		q->head = elem;
	q->tail       = elem;
	q->tail->next = NULL;
	++q->count;
}

static __inline__ void co_q_enq_head(co_queue_t *q, co_list_e_t *elem) {
	if (q->tail == NULL)
		q->tail = elem;
	elem->next = q->head;
	q->head    = elem;
	++q->count;
}

static __inline__ void co_q_deq(co_queue_t *q) {
	q->head = q->head->next;
	if (!q->head)
		q->tail = NULL;
	--q->count;
}

static __inline__ void co_q_cherry_pick(co_queue_t *q, co_list_e_t *elem) {
	co_list_e_t *iter = q->head, *prev = NULL;
	while (iter && iter != elem) {
		prev = iter;
		iter = iter->next;
	}
	if (!iter)
		return;
	if (!prev) {
		co_q_deq(q);
	} else {
		prev->next = iter->next;
		if (q->tail == iter)
			q->tail = prev;
		--q->count;
	}
}

static __inline__ co_list_e_t *co_q_peek(co_queue_t *q) { return q->head; }

static __inline__ void co_q_enq_q(co_queue_t *dst, co_queue_t *src) {
	if (dst->tail)
		dst->tail->next = src->head;
	else
		dst->head = src->head;
	dst->tail = src->tail;
	dst->count += src->count;
	*src = co_q_init();
}

#define for_each_co_list(var, head) for ((var) = (head); (var); (var) = (var)->next)

#define for_each_filter_list(var, prev, q)                                                                             \
	for ((var) = (q)->head, (prev) = NULL; (var); (prev) = (var), (var) = (var)->next)

#define for_each_drain_queue(var, q, peek, deq) for ((var) = peek(q); (var); (var) = peek(q), deq(q))

#endif /*CO_LIST_H*/