#include <criterion/criterion.h>
#include <math.h>
#include "../src/pqueue.h"

Test(pqueue, should_create_and_peek_single_element) {
    pqueue *pq = pq_create(5);
    cr_assert_not_null(pq, "Failed to create priority queue");

    int dummy = 42;
    bool enq_ok = pq_enqueue(pq, 3.14f, &dummy);
    cr_assert(enq_ok, "Enqueue failed on empty queue");

    pq_element top;
    bool peek_ok = pq_peek(pq, &top);
    cr_assert(peek_ok, "Peek failed on non-empty queue");
    cr_assert_float_eq(top.priority, 3.14f, 0.0001f);
    cr_assert_eq(top.content, &dummy);

    pq_destroy(pq);
}

Test(pqueue, should_dequeue_in_min_heap_order) {
    pqueue *pq = pq_create(10);

    int val1 = 1, val2 = 2, val3 = 3, val4 = 4;
    // Insert in unsorted order
    pq_enqueue(pq, 5.0f, &val1);
    pq_enqueue(pq, 1.2f, &val2);
    pq_enqueue(pq, 9.8f, &val3);
    pq_enqueue(pq, 0.5f, &val4);

    pq_element out;

    // 1st: smallest priority is 0.5 (val4)
    cr_assert(pq_dequeue(pq, &out));
    cr_assert_float_eq(out.priority, 0.5f, 0.0001f);
    cr_assert_eq(out.content, &val4);

    // 2nd: 1.2 (val2)
    cr_assert(pq_dequeue(pq, &out));
    cr_assert_float_eq(out.priority, 1.2f, 0.0001f);
    cr_assert_eq(out.content, &val2);

    // 3rd: 5.0 (val1)
    cr_assert(pq_dequeue(pq, &out));
    cr_assert_float_eq(out.priority, 5.0f, 0.0001f);
    cr_assert_eq(out.content, &val1);

    // 4th: 9.8 (val3)
    cr_assert(pq_dequeue(pq, &out));
    cr_assert_float_eq(out.priority, 9.8f, 0.0001f);
    cr_assert_eq(out.content, &val3);

    // Queue should now be empty
    cr_assert_not(pq_dequeue(pq, &out), "Dequeue should return false on empty queue");

    pq_destroy(pq);
}

Test(pqueue, should_reject_when_capacity_reached) {
    pqueue *pq = pq_create(2);

    int a = 1, b = 2, c = 3;
    cr_assert(pq_enqueue(pq, 1.0f, &a));
    cr_assert(pq_enqueue(pq, 2.0f, &b));

    // Third item must be rejected
    bool overflow_ok = pq_enqueue(pq, 3.0f, &c);
    cr_assert_not(overflow_ok, "Enqueue must return false when queue is full");

    pq_destroy(pq);
}

Test(pqueue, should_handle_empty_safely) {
    pqueue *pq = pq_create(3);
    pq_element out;

    cr_assert_not(pq_peek(pq, &out), "Peek must return false when queue is empty");
    cr_assert_not(pq_dequeue(pq, &out), "Dequeue must return false when queue is empty");

    pq_destroy(pq);
}

Test(pqueue, should_reject_nan_priority) {
    pqueue *pq = pq_create(5);
    int dummy = 42;
    
    // NAN is defined in math.h
    bool enq_ok = pq_enqueue(pq, NAN, &dummy);
    cr_assert_not(enq_ok, "Enqueue should reject NaN priorities");
    
    cr_assert_eq(pq_get_count(pq), 0, "Queue count should not increase on rejected enqueue");
    
    pq_destroy(pq);
}

Test(pqueue, should_clear_queue_successfully) {
    pqueue *pq = pq_create(5);
    int dummy = 42;
    
    pq_enqueue(pq, 1.0f, &dummy);
    pq_enqueue(pq, 2.0f, &dummy);
    cr_assert_eq(pq_get_count(pq), 2);
    
    bool clear_ok = pq_clear(pq);
    cr_assert(clear_ok, "pq_clear should return true on success");
    cr_assert_eq(pq_get_count(pq), 0, "Queue count should be 0 after clear");
    
    // Ensure we can enqueue again safely after clearing
    pq_enqueue(pq, 3.0f, &dummy);
    cr_assert_eq(pq_get_count(pq), 1);
    
    pq_destroy(pq);
}

Test(pqueue, should_handle_null_arguments_safely) {
    pq_element out;
    int dummy = 42;
    
    cr_assert_not(pq_enqueue(NULL, 1.0f, &dummy));
    cr_assert_not(pq_dequeue(NULL, &out));
    cr_assert_not(pq_peek(NULL, &out));
    cr_assert_not(pq_clear(NULL));
    
    cr_assert_eq(pq_get_count(NULL), -1);
    cr_assert_eq(pq_get_capacity(NULL), -1);
    
    // Test valid queue but NULL out parameter
    pqueue *pq = pq_create(5);
    pq_enqueue(pq, 1.0f, &dummy);
    cr_assert_not(pq_dequeue(pq, NULL));
    cr_assert_not(pq_peek(pq, NULL));
    
    pq_destroy(pq);
}

Test(pqueue, should_reject_invalid_capacity) {
    pqueue *pq_zero = pq_create(0);
    cr_assert_null(pq_zero, "pq_create should return NULL for capacity 0");
    
    pqueue *pq_neg = pq_create(-5);
    cr_assert_null(pq_neg, "pq_create should return NULL for negative capacity");
}
