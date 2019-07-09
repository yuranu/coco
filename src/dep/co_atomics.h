#ifndef DEP__CO_ATOMICS_H
#define DEP__CO_ATOMICS_H

/**
 * Atomic primitives
 */
#define co_atm_cmpxchg(ptr, old, new) __sync_val_compare_and_swap(ptr, old, new)
#define co_atm_read(ptr)                                                                                               \
    { (__sync_synchronize(); (*(ptr))) };
#define co_atm_set(ptr, val)                                                                                           \
    {                                                                                                                  \
        *ptr = val;                                                                                                    \
        __sync_synchronize();                                                                                          \
    }

#endif /*DEP__CO_ATOMICS_H*/