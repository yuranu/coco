#ifndef CO_ALLOC_H
#define CO_ALLOC_H

#include <stdlib.h>

#define co_malloc(...) malloc(__VA_ARGS__)
#define co_free(...) free(__VA_ARGS__)

static __inline__ void *co_malloc_memalign(size_t align, size_t size) {
    void *ptr;
    if (!posix_memalign(&ptr, align, size))
        return ptr;
    return NULL;
}

#endif /*CO_ALLOC_H*/