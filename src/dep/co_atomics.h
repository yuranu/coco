#ifndef DEP__CO_ATOMICS_H
#define DEP__CO_ATOMICS_H

#include "co_alloc.h"

/**
 * Atomic primitives
 */

/* clang-format off */
typedef struct { int counter; } co_atom_t __attribute__ ((aligned (32)));
#define co_atom_init(val) (co_atom_t){ (val) }

#define co_atom_arr_decl(name, size) co_atom_t name[size] __attribute__ ((aligned (32)));
#define co_atom_alloc(count) co_malloc_memalign(32, count * sizeof(co_atom_t))
#define co_atom_free(ptr) co_free(ptr)

#define co_atom_xchg(ptr, val) __sync_lock_test_and_set(&(ptr)->counter, (val))
#define co_atom_xchg_unlock(ptr) __sync_lock_release(&(ptr)->counter)
#define co_atom_cmpxchg(ptr, old, new) __sync_val_compare_and_swap(&(ptr)->counter, (old), (new))
#define co_atom_read(ptr) {(__sync_synchronize(); ((ptr)->counter))};
#define co_atom_set(ptr, val) { (ptr)->counter = (val); __sync_synchronize(); }
/* clang-format on */

#endif /*DEP__CO_ATOMICS_H*/