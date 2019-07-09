#ifndef CO_CHANNEL_H
#define CO_CHANNEL_H

#include "dep/co_alloc.h"
#include "dep/co_assert.h"
#include "dep/co_atomics.h"
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

/**
 * Unique channel id
 * =================
 * This aims to provide the following code functionality:
 * 1) If some thread aquired a channel, it is guaranteed no other thread will be able to
 *    aquire it until channel is released.
 * 2) It is an easy task if channel ID can be arbitrary large number, but what we want
 *    is actually the inverse: very small range (3-6 bits, maybe even 1 bit as a special a case).
 *    Hence threads will content over aquiring a channel. The assumption is that on average load,
 *    there are enough channels to provide for all threads needs without too much collisions.
 * 
 * In kernel mode channel is implemented straightforward as cpu_id + disable preemption (i.e. get_cpu()).
 * (+ optional IRQ disable if work queue can be used from interrupt context).
 *
 * In user mode, it is more complex as thread is not guaranteed not to change CPU at
 * any point. The design is like this, for N channels (for optimal performance N
 * should be ~ number of CPUs):
 *  1) Have a unique ID for each thread (tid) != 0.
 *  2) Maintain N atomic vars a[N], (each var large enough to contain a tid). Initial value 0: not aquired.
 *  3) Have a hash function f : tid -> (0..N-1)
 *  4) On aquire:
 *     - Start with a hash h = f(tid).
 *     - Atomic compare exchange value at a[h] with tid (if a[h] == 0)
 *     - On success channel = h
 *     - On failure, try h + 1, h + 2 ... up to X (suggested value X = N) times
 *  5) On release:
 *     - Atomic set a[channel] = 0;
 *  TODO:
 *  Alternatve implementations:
 *  1) Special case X = 1 - no retry on failure
 *  2) Implement whole system using N futexes
 */

/*
  User space implemetation of channel aquisition algorithm
  ========================================================
*/

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

/**
 * Unique task id.
 */
typedef pthread_t co_tid_t;

#define CO_RESERVED_TID 0

/**
 * Unique task id as atomic var.
 */
typedef co_tid_t co_atm_tid_t;

/**
 * Channel id type
 */
typedef unsigned int co_ch_t;

/**
 * Channel provider, used as context for all channel operations
 */
typedef struct co_ch_provider {
    co_atm_tid_t *a;
    co_ch_t max_ch;
} co_ch_provider_t;

/**
 * Initialize channel provider
 * @return 0 on success or error code on error
 */
static __inline__ int co_init_ch_provider(co_ch_provider_t *cp, co_ch_t max_ch) {
    cp->a = (co_atm_tid_t *)co_malloc_memalign(sizeof(void *), sizeof(co_atm_tid_t) * max_ch);
    return cp->a ? 0 : -ENOMEM;
}

/**
 * Destroy channel provider
 * @return 0 on success or error code on error
 */
static __inline__ void co_destroy_ch_provider(co_ch_provider_t *cp) {
    co_assert(cp->a, "Destroy uninitialized channel provider\n");
    free(cp->a);
}

/**
 * Get tid. In user space simple call to gettid system call
 * Not sure it cannot return 0. If it fails - find a different approach
 */
static __inline__ co_tid_t co_gettid() {
    co_tid_t tid = pthread_self();
    co_assert(tid != 0, "co_gettid() returned reserved value\n");
    return tid;
}

/**
 * Simple Knuth multiplication for now
 * @param key Input key
 * @param max Maximum return value, assume power of 2
 */
static __inline__ co_ch_t co_tid_hash(co_tid_t key, co_tid_t max) {
    return ((2654435769UL * (unsigned long)key) >> 32) & (max - 1);
}

/**
 * Aquire channel
 * @param cp Channel_provider
 * @return Channel id or negative error code
 */
static __inline__ co_ch_t co_aquire_ch_(co_ch_provider_t *cp) {
    co_tid_t tid = co_gettid();
    co_ch_t ch = co_tid_hash(tid, cp->max_ch);
    int retry;
    co_assert(ch >= 0 && ch < cp->max_ch, "Outbound hash value - bad hash function");
    for (retry = cp->max_ch; retry; --retry) {
        if (co_atm_cmpxchg(&cp->a[ch], CO_RESERVED_TID, tid) == CO_RESERVED_TID) {
            return ch;
        } else {
            if (++ch == cp->max_ch)
                ch = 0;
        }
    }
    return -EAGAIN;
}

/**
 * Release channel
 * @param cp Channel_provider
 * @param ch Channel id
 */
static __inline__ void co_release_ch_(co_ch_provider_t *cp, co_ch_t ch) { co_atm_set(&cp->a[ch], CO_RESERVED_TID); }

/**
 * Global channel provider, used as a shortcut for most channel ops
 *
 */
#define co_use_glob_ch_provider co_ch_provider_t __co_glob_ch_provider __attribute__((visibility("default")));
extern co_ch_provider_t __co_glob_ch_provider;
#define co_aquire_ch() co_aquire_ch_(__co_glob_ch_provider)
#define co_release_ch(ch) co_aquire_ch_(__co_glob_ch_provider, ch)

#endif /*CO_CHANNEL_H*/