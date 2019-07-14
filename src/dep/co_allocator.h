#ifndef CO_ALLOCATOR_H
#define CO_ALLOCATOR_H
/**
 * @file co_allocator.h
 *
 * Abstract interface for memory allocators used by coroutines
 *
 */

/**
 * Memory allocation management pitfall
 * ------------------------------------
 *
 * Memory allocation may become a bottleneck if poorly implemented. Problem is, that
 * each coroutine start requires creating an object, hence memory allocation.
 *
 * The idea is to delegate all memory allocations to a separate module, that can
 * be adjusted with accordance to specific needs.
 *
 * There is an assumption that root coroutine invocation may be a little slow, and this we
 * will tolerate. When a user invokes coroutine, he knows it will not execute immidiately,
 * rather start in a different thread.
 *
 * If, however, a coroutine invokes inner coroutine, there is no reason for it to be slow,
 * as we are in essentially a single threaded system.
 *
 * Hence we will differenciate between objects allocated by external allocator, and
 * objects allocated from a coroutine context, this allows optimizations to be done
 * based on single threaded execution properties of coroutines.
 *
 */

#include "co_types.h"

/**
 * Abstract allocator interface
 */
struct co_allocator;
typedef struct co_allocator {
	void *(*alloc)(struct co_allocator *, co_size_t);
	void (*free)(struct co_allocator *, void *);
} co_allocator_t;

#endif /*CO_ALLOCATOR_H*/