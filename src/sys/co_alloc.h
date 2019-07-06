#ifndef SYS__CO_ALLOC_H
#define SYS__CO_ALLOC_H

/**
 * This file declares the minimal memory allocation interface required for
 * correct operation of the library.
 * Currently, minimal functionality is:
 * > Allocate object
 * > Free object
 * > Allocate memory of size
 * > Free memory of size
 * TODO: Currently using stdlib, later split this file into different implementations,
 *       based on compilation flags.
 * TODO: Create a wrapper above this file, that will exploit the fact that allocations are
 *       constant size. It will create large pools of various sizes, possibly pre-set
 *       some initial values, and provide objects from pools as needed without the need to
 *       perform memory allocation.
 * @author: Yuri Nudelman (yuranu@gmail.com)
 */

#include <stdlib.h>

/**
 * Allocate context object
 */
#define co_ctx_alloc(ctx) co_mem_alloc(sizeof(*ctx))

/**
 * Free context object
 */
#define co_ctx_free(ctx) co_mem_free(ctx)

#define co_mem_alloc malloc
#define co_mem_free free

#endif /*SYS__CO_ALLOC_H*/