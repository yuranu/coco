#ifndef SYS__CO_THREADS_H
#define SYS__CO_THREADS_H

/**
 * This file declares the minimal threads and synchronization interface required for
 * correct operation of the library.
 * Currently, minimal functionality is:
 * > Semaphores
 * TODO: Currently using POSIX, later split this file into different implementations,
 *       based on compilation flags.
 * @author: Yuri Nudelman (yuranu@gmail.com)
 */

#include <errno.h>
#include <semaphore.h>

/**
 * --- Condition variables ---
 */

/**
 * Declare semaphore @sem
 */
#define co_sem_define(sem) sem_t sem

/**
 * Initialize semaphore.
 * @return: 0 on success, error code otherwise
 */
#define co_sem_init(sem)                                                                                               \
    ({                                                                                                                 \
        int __sem_rv = sem_init(&sem, 0, 0);                                                                           \
        __sem_rv = !__sem_rv ? 0 : (errno ? errno : -1);                                                               \
        __sem_rv;                                                                                                      \
    })

/**
 * Destroy semaphore.
 * @return: 0 on success, error code otherwise
 */
#define co_sem_destroy(sem)                                                                                            \
    ({                                                                                                                 \
        int __sem_rv = sem_destroy(&sem);                                                                              \
        __sem_rv = !__sem_rv ? 0 : (errno ? errno : -1);                                                               \
        __sem_rv;                                                                                                      \
    })

/**
 * Increment semaphore.
 * @return: 0 on success, error code otherwise
 */
#define co_sem_up(sem)                                                                                                 \
    ({                                                                                                                 \
        int __sem_rv = sem_post(&sem);                                                                                 \
        __sem_rv = !__sem_rv ? 0 : (errno ? errno : -1);                                                               \
        __sem_rv;                                                                                                      \
    })

/**
 * Decrement semaphore and wait if needed.
 * @return: 0 on success, error code otherwise
 */
#define co_sem_down(sem)                                                                                               \
    ({                                                                                                                 \
        int __sem_rv = sem_wait(&sem);                                                                                 \
        __sem_rv = !__sem_rv ? 0 : (errno ? errno : -1);                                                               \
        __sem_rv;                                                                                                      \
    })

#endif /*SYS__CO_THREADS_H*/
