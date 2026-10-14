#include <criterion/criterion.h>
#include <math.h>
#include "../src/pqueue.h"

static bool is_min_heap(float a, float b) { return a < b; }

Test(pqueue, should_create_and_peek_single_element) {
    pqueue *pq = NULL;
    cr_assert_eq(pq_create(5, is_min_heap, &pq), PQ_OK);
    cr_assert_not_null(pq, "Failed to create priority queue");

    int dummy = 42;
    pq_state enq_ok = pq_enqueue(pq, 3.14f, &dummy);
    cr_assert_eq(enq_ok, PQ_OK, "Enqueue failed on empty queue");

    pq_element top;
    bool peek_ok = pq_peek(pq, &top);
    cr_assert(peek_ok, "Peek failed on non-empty queue");
    cr_assert_float_eq(top.priority, 3.14f, 0.0001f);
    cr_assert_eq(top.content, &dummy);

    pq_destroy(pq);
}

Test(pqueue, should_dequeue_in_min_heap_order) {
    pqueue *pq = NULL;
    pq_create(10, is_min_heap, &pq);

    int val1 = 1, val2 = 2, val3 = 3, val4 = 4;
    pq_enqueue(pq, 5.0f, &val1);
    pq_enqueue(pq, 1.2f, &val2);
    pq_enqueue(pq, 9.8f, &val3);
    pq_enqueue(pq, 0.5f, &val4);

    pq_element out;

    cr_assert(pq_dequeue(pq, &out));
    cr_assert_float_eq(out.priority, 0.5f, 0.0001f);
    cr_assert_eq(out.content, &val4);

    cr_assert(pq_dequeue(pq, &out));
    cr_assert_float_eq(out.priority, 1.2f, 0.0001f);
    cr_assert_eq(out.content, &val2);

    cr_assert(pq_dequeue(pq, &out));
    cr_assert_float_eq(out.priority, 5.0f, 0.0001f);
    cr_assert_eq(out.content, &val1);

    cr_assert(pq_dequeue(pq, &out));
    cr_assert_float_eq(out.priority, 9.8f, 0.0001f);
    cr_assert_eq(out.content, &val3);

    cr_assert_not(pq_dequeue(pq, &out), "Dequeue should return false on empty queue");

    pq_destroy(pq);
}

Test(pqueue, should_reject_when_capacity_reached) {
    pqueue *pq = NULL;
    pq_create(2, is_min_heap, &pq);

    int a = 1, b = 2, c = 3;
    cr_assert_eq(pq_enqueue(pq, 1.0f, &a), PQ_OK);
    cr_assert_eq(pq_enqueue(pq, 2.0f, &b), PQ_OK);

    pq_state overflow_ok = pq_enqueue(pq, 3.0f, &c);
    cr_assert_eq(overflow_ok, PQ_ERROR_FULL, "Enqueue must return PQ_ERROR_FULL when queue is full");

    pq_destroy(pq);
}

Test(pqueue, should_handle_empty_safely) {
    pqueue *pq = NULL;
    pq_create(3, is_min_heap, &pq);
    pq_element out;

    cr_assert_not(pq_peek(pq, &out), "Peek must return false when queue is empty");
    cr_assert_not(pq_dequeue(pq, &out), "Dequeue must return false when queue is empty");

    pq_destroy(pq);
}

Test(pqueue, should_reject_nan_priority) {
    pqueue *pq = NULL;
    pq_create(5, is_min_heap, &pq);
    int dummy = 42;
    
    pq_state enq_ok = pq_enqueue(pq, NAN, &dummy);
    cr_assert_eq(enq_ok, PQ_ERROR_INVALID_ARGUMENT, "Enqueue should reject NaN priorities");
    
    int count;
    pq_get_count(pq, &count);
    cr_assert_eq(count, 0, "Queue count should not increase on rejected enqueue");
    
    pq_destroy(pq);
}

Test(pqueue, should_clear_queue_successfully) {
    pqueue *pq = NULL;
    pq_create(5, is_min_heap, &pq);
    int dummy = 42;
    int count;
    
    pq_enqueue(pq, 1.0f, &dummy);
    pq_enqueue(pq, 2.0f, &dummy);
    
    pq_get_count(pq, &count);
    cr_assert_eq(count, 2);
    
    bool clear_ok = pq_clear(pq);
    cr_assert(clear_ok, "pq_clear should return true on success");
    
    pq_get_count(pq, &count);
    cr_assert_eq(count, 0, "Queue count should be 0 after clear");
    
    pq_enqueue(pq, 3.0f, &dummy);
    pq_get_count(pq, &count);
    cr_assert_eq(count, 1);
    
    pq_destroy(pq);
}

Test(pqueue, should_handle_null_arguments_safely) {
    pq_element out;
    int dummy = 42;
    int val;
    
    cr_assert_neq(pq_enqueue(NULL, 1.0f, &dummy), PQ_OK);
    cr_assert_not(pq_dequeue(NULL, &out));
    cr_assert_not(pq_peek(NULL, &out));
    cr_assert_not(pq_clear(NULL));
    
    cr_assert_not(pq_get_count(NULL, &val));
    cr_assert_not(pq_get_capacity(NULL, &val));
    
    pqueue *pq = NULL;
    pq_create(5, is_min_heap, &pq);
    pq_enqueue(pq, 1.0f, &dummy);
    cr_assert_not(pq_dequeue(pq, NULL));
    cr_assert_not(pq_peek(pq, NULL));
    
    pq_destroy(pq);
}

Test(pqueue, should_reject_invalid_capacity) {
    pqueue *pq = NULL;
    
    cr_assert_eq(pq_create(0, is_min_heap, &pq), PQ_ERROR_INVALID_ARGUMENT);
    cr_assert_eq(pq_create(-5, is_min_heap, &pq), PQ_ERROR_INVALID_ARGUMENT);
}
