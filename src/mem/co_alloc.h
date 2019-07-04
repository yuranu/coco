#ifndef UTILS__CO_ALLOC_H
#define UTILS__CO_ALLOC_H

#include <stdlib.h>

#define co_ctx_alloc(ctx) malloc(sizeof(*ctx))
#define co_ctx_free(ctx) free(ctx)

#endif /*UTILS__CO_ALLOC_H*/