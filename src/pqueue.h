#pragma once
#include <stdbool.h>

// PQUEUE: a simple, non-thread-safe, fixed-capacity binary min-heap
// priority queue. Elements are dequeued in ascending order of priority.
// Capacity is fixed at creation; there is currently no growable variant.

// A single entry in the queue.
//   priority - orders the element; lower values are dequeued first.
//              Must not be NaN (pq_enqueue rejects it).
//   content  - an opaque, caller-owned pointer. The queue never
//              dereferences, copies, or frees it.
typedef struct pq_element {
    float priority;
    void* content;
} pq_element;

// Opaque handle to a priority queue instance.
typedef struct pqueue pqueue;

// Allocates a queue with room for exactly `capacity` elements.
// Returns NULL if capacity <= 0 or on allocation failure.
pqueue* pq_create(int capacity);

// Frees the queue and its internal storage. Does NOT free any
// remaining `content` pointers still in the queue — the caller
// is responsible for those. Safe to call with pq == NULL (no-op).
void pq_destroy(pqueue* pq);

// Inserts `content` with the given `priority`.
// The queue stores `content` as an opaque handle and never
// dereferences, copies, or frees it; the caller must keep the
// pointed-to object alive until it is dequeued.
// Returns false if pq is NULL, the queue is full, or priority is NaN.
bool pq_enqueue(pqueue* pq, float priority, void* content);

// Removes and returns (via *out_item) the element with the lowest
// priority. Returns false if pq is NULL, out_item is NULL, or the
// queue is empty (*out_item is left unmodified in that case).
bool pq_dequeue(pqueue* pq, pq_element* out_item);

// Copies the lowest-priority element into *out_item without
// removing it. Same failure conditions as pq_dequeue.
bool pq_peek(const pqueue* pq, pq_element* out_item);

// Returns the number of elements currently queued, or -1 if pq is NULL.
int pq_get_count(const pqueue* pq);

// Returns the queue's fixed capacity, or -1 if pq is NULL.
int pq_get_capacity(const pqueue* pq);

// Removes all elements without freeing their `content`.
// Returns false if pq is NULL.
bool pq_clear(pqueue* pq);
