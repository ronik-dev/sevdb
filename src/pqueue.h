#pragma once
#include <stdbool.h>

/**
 * @struct pq_element
 * @brief A single entry in the priority queue.
 * 
 * @var pq_element::priority 
 * Orders the element based on the queue's comparator. Must not be NaN.
 * @var pq_element::content 
 * An opaque, caller-owned pointer. The queue never dereferences, copies, or frees it.
 */
typedef struct pq_element {
    float priority;
    void* content;
} pq_element;

/**
 * @brief Comparator function pointer type for heap ordering.
 * 
 * Returns true if 'a' should be placed closer to the top of the heap than 'b'.
 * Examples:
 * - Min-Heap (e.g., Euclidean distance): `return a < b;`
 * - Max-Heap (e.g., Cosine similarity):  `return a > b;`
 */
typedef bool(*pq_compare_fn)(float a, float b);

/**
 * @struct pqueue
 * @brief Opaque handle to a generic, non-thread-safe, fixed-capacity binary heap.
 * 
 * Elements are dequeued based on the rules established by the `pq_compare_fn` 
 * provided at creation. Capacity is fixed; there is no growable variant.
 */
typedef struct pqueue pqueue;
  

/**
 * @brief Allocates a priority queue with room for exactly `capacity` elements.
 * 
 * @param capacity The maximum number of elements. Must be > 0.
 * @param comparator The function used to order the heap.
 * @return A pointer to the newly allocated queue, or NULL on failure.
 */
pqueue* pq_create(int capacity, pq_compare_fn comparator);

/**
 * @brief Frees the queue and its internal array storage. 
 * 
 * @note Does NOT free any remaining `content` pointers still in the queue; 
 *       the caller is strictly responsible for managing their own memory.
 * @param pq Pointer to the queue. Safe to call with NULL (no-op).
 */
void pq_destroy(pqueue* pq);

/**
 * @brief Inserts `content` with the given `priority` into the heap.
 * 
 * @param pq Pointer to the queue.
 * @param priority The numeric priority to sort by. Must not be NaN.
 * @param content Opaque pointer to the caller's data. Must be kept alive by the caller.
 * @return True on success, False if the queue is NULL, full, or priority is NaN.
 */
bool pq_enqueue(pqueue* pq, float priority, void* content);

/**
 * @brief Removes and returns the highest-ranking element in the queue.
 * 
 * @param pq Pointer to the queue.
 * @param out_item Caller-allocated struct to be populated with the dequeued element.
 * @return True on success. False if the queue is empty, NULL, or out_item is NULL 
 *         (*out_item is left unmodified).
 */
bool pq_dequeue(pqueue* pq, pq_element* out_item);

/**
 * @brief Copies the highest-ranking element into *out_item without removing it.
 * 
 * @param pq Pointer to the queue.
 * @param out_item Caller-allocated struct to be populated with the element.
 * @return True on success. False if the queue is empty, NULL, or out_item is NULL.
 */
bool pq_peek(const pqueue* pq, pq_element* out_item);

/**
 * @brief Retrieves the number of elements currently stored in the queue.
 * 
 * @param pq Pointer to the queue.
 * @return The element count, or -1 if pq is NULL.
 */
int pq_get_count(const pqueue* pq);

/**
 * @brief Retrieves the maximum capacity of the queue.
 * 
 * @param pq Pointer to the queue.
 * @return The fixed capacity, or -1 if pq is NULL.
 */
int pq_get_capacity(const pqueue* pq);

/**
 * @brief Empties the queue by resetting the count to zero.
 * 
 * @note Does not free the `content` pointers of the discarded elements.
 * @param pq Pointer to the queue.
 * @return True on success, False if pq is NULL.
 */
bool pq_clear(pqueue* pq);
