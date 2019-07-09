#ifndef CO_MULTI_SRC_Q_H
#define CO_MULTI_SRC_Q_H

/**
 * Multi source queue
 * ==================
 * The aim of this module is to provide a lockless queue that can receive input
 * from multiple threads (enque) and provide output to a single thread (deque).
 * 
 * We rely on atomic operation for implementation. There is an assumption that
 * the load is low enough to allow smooth lockless operation of the queu in good path,
 * with rare collision handling. The aim is to achieve good *average* runtime.
 * 
 * 
 * !!! IMPORTANT: Memory allocation is an issue and may become a bottleneck.
 * In context of this module, it is just assumed memory allocation *magically* works.
 * How it is done is left to an allocator to solve.
 * 
 * The idea:
 * 
 * There are reader and many writers. Writers' aim is to put an element into
 * an intermediate queue, as quickly as possible.
 * Reader's aim is to empty all intermediate queues.
 * The extra elements can be then moved to a normal synchronous queue and analyzed calmly.
 * 
 * 1) Relies on channels implemention @see src/co_channel.h.
 * 2) Queue ement consists of data and next pointer.
 * 3) Maintain a simple synchronous queue, S.
 * 4) Maintain a small number (N) of per channel queues. Recommended N = number of CPUs.
 *    For each channel, save a queue head pointer. All new insertions are to the head.
 * 5) Convention: if tail.next == NULL, tail was not dequeued.
 * 6) On enqueue:
 *    - Allocate memory for queue element, E. Init data, init E.next = NULL.
 *    - Aquire a channel ch. From now on, we can be sure no other writer will touch channel
 *      queue.
 *    - Repeat:
 *      - Read ch.head -> head
 *      - E.next = head
 *      - Atomic compare exchange: if ch.head == head: ch.head = E
 *      - Until compare exchange success.
 * 7) On dequeue:
 *    - If we have some elements in S, return them first.
 *    - Else iterate over all channels. For each channel ch:
 *      Repeat:
 *      - Read ch->head -> head
 *        - If it is null - break, move to next channel.
 *        - Atomic compare exchange: if ch.head == head: ch.head = NULL
 *        - Until compare exchange success.
 *      - Take head, reverse queue order, put to S. Go back to S.
 */


#endif /*CO_MULTI_SRC_WQ_H*/