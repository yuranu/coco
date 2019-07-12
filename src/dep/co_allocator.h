#ifndef CO_ALLOCATOR_H
#define CO_ALLOCATOR_H
/**
 * @file co_allocator.h
 *
 * Abstract interface for memory allocators used by coroutines
 *
 */

#include "co_types.h"
struct co_allocator;
typedef struct co_allocator {
	void *(*alloc)(struct co_allocator *, co_size_t);
	void (*free)(struct co_allocator *, void *);
} co_allocator_t;

#endif /*CO_ALLOCATOR_H*/