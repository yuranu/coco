#ifndef CO_PRIMITIVE_ALLOCATOR_H
#define CO_PRIMITIVE_ALLOCATOR_H
/**
 * @file co_primitive_allocator.h
 *
 * Most straightforward allocator. Simply calls to malloc and free.
 * Should be used for testing or as a slow allocator.
 * For fast allocator, this one is far from optimal.
 *
 */

#include "co_alloc.h"

static void *co_primitive_allocator_alloc(struct co_allocator *a, co_size_t s) { return co_malloc(s); }
static void co_primitive_allocator_free(struct co_allocator *a, void *p) { co_free(p); }

#define co_primitive_allocator_init()                                                                                  \
	(struct co_allocator) { co_primitive_allocator_alloc, co_primitive_allocator_free }

#endif /*CO_PRIMITIVE_ALLOCATOR_H*/