#include <criterion/criterion.h>
#include "../src/hashmap.h"

Test(hashmap, should_create_and_destroy_successfully) {
    hashmap *hm = NULL;
    cr_assert_eq(hm_create(10, &hm), HM_OK, "Failed to create hashmap");
    cr_assert_not_null(hm, "Hashmap pointer is null after successful create");
    
    uint32_t capacity, count;
    cr_assert(hm_get_capacity(hm, &capacity));
    cr_assert(hm_get_count(hm, &count));
    cr_assert_eq(capacity, 10, "Capacity should match initialization");
    cr_assert_eq(count, 0, "Initial count should be 0");

    bool destroyed = hm_destroy(hm);
    cr_assert(destroyed, "Destroy failed");
}

Test(hashmap, should_insert_and_get_value) {
    hashmap *hm = NULL;
    hm_create(10, &hm);
    
    cr_assert_eq(hm_insert_new(hm, 42, 100), HM_OK, "Insert should succeed on empty map");
    cr_assert(hm_contains_key(hm, 42), "Contains should return true for inserted key");
    
    uint32_t count;
    hm_get_count(hm, &count);
    cr_assert_eq(count, 1, "Count should increase after insert");

    uint32_t out_val;
    cr_assert_eq(hm_get_value(hm, 42, &out_val), HM_OK, "Get should succeed for inserted key");
    cr_assert_eq(out_val, 100, "Retrieved value should match inserted value");

    hm_destroy(hm);
}

Test(hashmap, should_reject_duplicate_inserts) {
    hashmap *hm = NULL;
    hm_create(10, &hm);
    
    cr_assert_eq(hm_insert_new(hm, 7, 50), HM_OK);
    hm_state duplicate_insert = hm_insert_new(hm, 7, 60);
    
    cr_assert_eq(duplicate_insert, HM_ERROR_DUPLICATE_KEY, "Insert should reject duplicate keys");
    
    uint32_t count;
    hm_get_count(hm, &count);
    cr_assert_eq(count, 1, "Count should not increase on duplicate insert");

    uint32_t out_val;
    hm_get_value(hm, 7, &out_val);
    cr_assert_eq(out_val, 50, "Original value should remain unchanged after rejected insert");

    hm_destroy(hm);
}

Test(hashmap, should_update_existing_value) {
    hashmap *hm = NULL;
    hm_create(10, &hm);
    hm_insert_new(hm, 99, 1000);

    hm_state update_ok = hm_update_value(hm, 99, 2000);
    cr_assert_eq(update_ok, HM_OK, "Update should succeed for existing key");

    uint32_t out_val;
    hm_get_value(hm, 99, &out_val);
    cr_assert_eq(out_val, 2000, "Value should reflect the update");

    hm_state update_missing = hm_update_value(hm, 100, 3000);
    cr_assert_eq(update_missing, HM_ERROR_NOT_FOUND, "Update should fail for missing key");

    hm_destroy(hm);
}

Test(hashmap, should_survive_linear_probing_and_tombstones) {
    hashmap *hm = NULL;
    hm_create(5, &hm); 
    
    cr_assert_eq(hm_insert_new(hm, 0, 100), HM_OK);
    cr_assert_eq(hm_insert_new(hm, 5, 200), HM_OK);
    cr_assert_eq(hm_insert_new(hm, 10, 300), HM_OK);
    
    uint32_t count;
    hm_get_count(hm, &count);
    cr_assert_eq(count, 3);

    cr_assert_eq(hm_remove(hm, 5), HM_OK, "Remove should succeed for existing key");
    
    hm_get_count(hm, &count);
    cr_assert_eq(count, 2);
    cr_assert_not(hm_contains_key(hm, 5), "Removed key should no longer be found");

    uint32_t out_val;
    cr_assert_eq(hm_get_value(hm, 10, &out_val), HM_OK, "Probe chain must survive tombstones");
    cr_assert_eq(out_val, 300);

    cr_assert_eq(hm_insert_new(hm, 15, 400), HM_OK);
    cr_assert_eq(hm_get_value(hm, 15, &out_val), HM_OK);
    cr_assert_eq(out_val, 400);

    hm_destroy(hm);
}

Test(hashmap, should_grow_automatically_when_load_factor_exceeded) {
    uint32_t initial_capacity = 4;
    hashmap *hm = NULL;
    hm_create(initial_capacity, &hm);
    
    for (uint32_t i = 0; i < 10; i++) {
        cr_assert_eq(hm_insert_new(hm, i, i * 10), HM_OK, "Insert failed during heavy load at key %d", i);
    }
    
    uint32_t capacity, count;
    hm_get_capacity(hm, &capacity);
    hm_get_count(hm, &count);
    
    cr_assert(capacity > initial_capacity, "Capacity should have grown");
    cr_assert_eq(count, 10, "Count should be exactly 10");

    uint32_t out_val;
    for (uint32_t i = 0; i < 10; i++) {
        cr_assert_eq(hm_get_value(hm, i, &out_val), HM_OK);
        cr_assert_eq(out_val, i * 10, "Value corrupted during rehash for key %d", i);
    }

    hm_destroy(hm);
}

Test(hashmap, should_handle_null_arguments_safely) {
    uint32_t out_val;
    hashmap *null_hm = NULL;

    cr_assert_neq(hm_create(10, NULL), HM_OK);
    cr_assert_neq(hm_insert_new(NULL, 1, 100), HM_OK);
    cr_assert_neq(hm_update_value(NULL, 1, 200), HM_OK);
    cr_assert_not(hm_contains_key(NULL, 1));
    cr_assert_neq(hm_get_value(NULL, 1, &out_val), HM_OK);
    cr_assert_neq(hm_remove(NULL, 1), HM_OK);
    cr_assert_not(hm_destroy(NULL));

    hashmap *hm = NULL;
    hm_create(5, &hm);
    cr_assert_neq(hm_get_value(hm, 1, NULL), HM_OK, "Should reject NULL out_value pointer");
    hm_destroy(hm);
}

Test(hashmap, should_reject_invalid_capacity) {
    hashmap *hm = (hashmap*)0xDEADBEEF; // Dummy address
    cr_assert_eq(hm_create(0, &hm), HM_ERROR_INVALID_ARGUMENT, "hm_create should return error for capacity 0");
}

Test(hashmap, regression_should_recalculate_bucket_index_after_growth) {
    uint32_t initial_capacity = 4;
    hashmap *hm = NULL;
    cr_assert_eq(hm_create(initial_capacity, &hm), HM_OK);

    // Fill to the load factor threshold (4 * 0.7 = 2.8)
    // We need 3 elements so the count becomes 3, triggering growth on the 4th.
    cr_assert_eq(hm_insert_new(hm, 1, 10), HM_OK);
    cr_assert_eq(hm_insert_new(hm, 2, 20), HM_OK);
    cr_assert_eq(hm_insert_new(hm, 3, 30), HM_OK);

    // This 4th insert now triggers the capacity growth.
    uint32_t triggering_key = 4;
    cr_assert_eq(hm_insert_new(hm, triggering_key, 40), HM_OK);

    uint32_t capacity_after_growth;
    hm_get_capacity(hm, &capacity_after_growth);
    cr_assert(capacity_after_growth > initial_capacity, "Expected hashmap to grow");

    // Verify the triggering element is reachable at its correct rehashed index
    uint32_t val;
    cr_assert_eq(hm_get_value(hm, triggering_key, &val), HM_OK, 
                 "Triggering key must be retrievable after growth");
    cr_assert_eq(val, 40);

    // Insert subsequent elements that hash to the same bucket in both old and new capacities
    cr_assert_eq(hm_insert_new(hm, 4 + capacity_after_growth, 400), HM_OK);
    cr_assert_eq(hm_get_value(hm, 4 + capacity_after_growth, &val), HM_OK);
    cr_assert_eq(val, 400);

    hm_destroy(hm);
}

Test(hashmap, should_return_not_found_on_missing_key_operations) {
    hashmap *hm = NULL;
    hm_create(8, &hm);

    uint32_t val;
    cr_assert_eq(hm_get_value(hm, 1234, &val), HM_ERROR_NOT_FOUND, 
                 "hm_get_value should return HM_ERROR_NOT_FOUND for non-existent key");
    cr_assert_eq(hm_remove(hm, 1234), HM_ERROR_NOT_FOUND, 
                 "hm_remove should return HM_ERROR_NOT_FOUND for non-existent key");
    cr_assert_eq(hm_update_value(hm, 1234, 50), HM_ERROR_NOT_FOUND, 
                 "hm_update_value should return HM_ERROR_NOT_FOUND for non-existent key");

    hm_destroy(hm);
}

Test(hashmap, should_reuse_deleted_tombstone_slot) {
    hashmap *hm = NULL;
    hm_create(5, &hm);

    // Insert elements causing collision chaining
    cr_assert_eq(hm_insert_new(hm, 1, 100), HM_OK);
    cr_assert_eq(hm_insert_new(hm, 6, 200), HM_OK); // 6 % 5 == 1

    uint32_t initial_count;
    hm_get_count(hm, &initial_count);
    cr_assert_eq(initial_count, 2);

    // Remove key 1, creating a DELETED tombstone
    cr_assert_eq(hm_remove(hm, 1), HM_OK);

    // Re-inserting key 1 or another key hashing to bucket 1 should reclaim the tombstone
    cr_assert_eq(hm_insert_new(hm, 11, 300), HM_OK); // 11 % 5 == 1

    uint32_t val;
    cr_assert_eq(hm_get_value(hm, 11, &val), HM_OK);
    cr_assert_eq(val, 300);

    // Key 6 further down the probe chain must still remain intact
    cr_assert_eq(hm_get_value(hm, 6, &val), HM_OK);
    cr_assert_eq(val, 200);

    hm_destroy(hm);
}
