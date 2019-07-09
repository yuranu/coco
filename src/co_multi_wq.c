#include "co_channel.h"
#include <stdio.h>
#include <stdlib.h>
/**
 * Abstractions over malloc and free, hiding real implementation
 */

#define co_malloc(...) malloc(__VA_ARGS__)
#define co_free(...) free(__VA_ARGS__)

/*
  User space implemetation of multi work queue
  ============================================
*/

/**
 * Test code
 */

co_use_glob_ch_provider;

int main() {
    printf("%lu\n", sizeof(pid_t));
    return 0;
}