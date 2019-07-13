#ifndef CO_SYNC_H
#define CO_SYNC_H
/**
 * @file co_sync.h
 *
 * Minimal synchronization primitives interface used by the library
 *
 */

#include "co_atomics.h"
#include "co_dbg.h"
#include "co_types.h"
#include <pthread.h>
#include <semaphore.h>

/**
 * Semaphore type
 */
typedef sem_t co_sem_t;

/**
 * Initialize semaphore.
 * @param sem Semaphore pointer
 * @return: 0 or error code
 */
static __inline__ co_errno_t co_sem_init(co_sem_t *sem, int size) {
	int rv = sem_init(sem, 0, 0);
	rv     = !rv ? 0 : (errno ? errno : EINVAL);
	return -rv;
}

/**
 * Destroy semaphore.
 * @param sem Semaphore pointer
 */
static __inline__ void co_sem_destroy(co_sem_t *sem) { sem_destroy(sem); }

/**
 * Increment semaphore.
 * @param sem Semaphore pointer
 * @return: 0 or error code
 */
static __inline__ co_errno_t co_sem_up(co_sem_t *sem) {
	int rv = sem_post(sem);
	rv     = !rv ? 0 : (errno ? errno : EINVAL);
	return -rv;
}

/**
 * Decrement semaphore and wait if needed.
 * @param sem Semaphore pointer
 * @return: 0 or error code
 */
static __inline__ co_errno_t co_sem_down(co_sem_t *sem) {
	int rv = sem_wait(sem);
	rv     = !rv ? 0 : (errno ? errno : EINVAL);
	return -rv;
}

/**
 * Functionality similar to kernel completion object
 * Iplemented with conditional variables
 */
typedef struct co_completion {
	co_bool_t done;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} co_completion_t;

/**
 * Initialize completion.
 * @param comp Completion pointer
 * @return: 0 or error code
 */
static __inline__ co_errno_t co_completion_init(co_completion_t *comp) {
	comp->done = 0;
	int rv     = -pthread_cond_init(&comp->cond, NULL);
	if (rv)
		return rv;
	rv = -pthread_mutex_init(&comp->mutex, NULL);
	if (rv)
		pthread_cond_destroy(&comp->cond);
	return rv;
}

/**
 * Destroy completion.
 * @param comp Completion pointer
 * @return: 0 or error code
 */
static __inline__ void co_completion_destroy(co_completion_t *comp) {
	pthread_cond_destroy(&comp->cond);
	pthread_mutex_destroy(&comp->mutex);
}

/**
 * Wait for completion.
 * @param comp Completion pointer
 * @return: 0 or error code
 */
static __inline__ co_errno_t co_completion_wait(co_completion_t *comp) {
	int rv = -pthread_mutex_lock(&comp->mutex);
	if (rv)
		return rv;
	while (!comp->done) {
		rv = -pthread_cond_wait(&comp->cond, &comp->mutex);
		if (rv)
			return rv;
	}
	co_dbg_trace("unset completion\n");
	comp->done = 0;
	return -pthread_mutex_unlock(&comp->mutex);
}

/**
 * Notify completion.
 * @param comp Completion pointer
 * @return: 0 or error code
 */
static __inline__ co_errno_t co_completion_done(co_completion_t *comp) {
	int rv = -pthread_mutex_lock(&comp->mutex);
	if (rv)
		return rv;
	rv = pthread_cond_signal(&comp->cond);
	if (rv)
		return rv;
	co_dbg_trace("set completion\n");
	comp->done = 1;
	return -pthread_mutex_unlock(&comp->mutex);
}

/**
 * Get a not necesserilly unique but consistent per thread hash code
 * @return: 0 or error code
 */
static __inline__ co_size_t co_tid_hash() { return ((2654435769UL * (unsigned long)pthread_self()) >> 32); }

#endif /*CO_SYNC_H*/
