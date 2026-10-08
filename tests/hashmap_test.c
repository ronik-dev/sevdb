#include <criterion/criterion.h>
#include "../src/hashmap.h"

Test(hashmap, should_create_and_destroy_successfully) {
    hashmap *hm = hm_create(10);
    cr_assert_not_null(hm, "Failed to create hashmap");
    cr_assert_eq(hm->capacity, 10, "Capacity should match initialization");
    cr_assert_eq(hm->count, 0, "Initial count should be 0");

    bool destroyed = hm_destroy(hm);
    cr_assert(destroyed, "Destroy failed");
}

Test(hashmap, should_insert_and_get_value) {
    hashmap *hm = hm_create(10);
    
    cr_assert(hm_insert_new(hm, 42, 100), "Insert should succeed on empty map");
    cr_assert(hm_contains_key(hm, 42), "Contains should return true for inserted key");
    cr_assert_eq(hm->count, 1, "Count should increase after insert");

    uint32_t out_val;
    cr_assert(hm_get_value(hm, 42, &out_val), "Get should succeed for inserted key");
    cr_assert_eq(out_val, 100, "Retrieved value should match inserted value");

    hm_destroy(hm);
}

Test(hashmap, should_reject_duplicate_inserts) {
    hashmap *hm = hm_create(10);
    
    cr_assert(hm_insert_new(hm, 7, 50));
    bool duplicate_insert = hm_insert_new(hm, 7, 60);
    
    cr_assert_not(duplicate_insert, "Insert should reject duplicate keys");
    cr_assert_eq(hm->count, 1, "Count should not increase on duplicate insert");

    uint32_t out_val;
    hm_get_value(hm, 7, &out_val);
    cr_assert_eq(out_val, 50, "Original value should remain unchanged after rejected insert");

    hm_destroy(hm);
}

Test(hashmap, should_update_existing_value) {
    hashmap *hm = hm_create(10);
    hm_insert_new(hm, 99, 1000);

    bool update_ok = hm_update_value(hm, 99, 2000);
    cr_assert(update_ok, "Update should succeed for existing key");

    uint32_t out_val;
    hm_get_value(hm, 99, &out_val);
    cr_assert_eq(out_val, 2000, "Value should reflect the update");

    bool update_missing = hm_update_value(hm, 100, 3000);
    cr_assert_not(update_missing, "Update should fail for missing key");

    hm_destroy(hm);
}

Test(hashmap, should_survive_linear_probing_and_tombstones) {
    hashmap *hm = hm_create(5); 
    // Keys 0, 5, and 10 will all hash to bucket 0 (X % 5 == 0).
    // This forces linear probing collisions.
    
    cr_assert(hm_insert_new(hm, 0, 100));
    cr_assert(hm_insert_new(hm, 5, 200));
    cr_assert(hm_insert_new(hm, 10, 300));
    cr_assert_eq(hm->count, 3);

    // Remove the middle element in the probe chain (key 5)
    cr_assert(hm_remove(hm, 5), "Remove should succeed for existing key");
    cr_assert_eq(hm->count, 2);
    cr_assert_not(hm_contains_key(hm, 5), "Removed key should no longer be found");

    // Ensure the tombstone (DELETED) didn't break the probe chain for key 10
    uint32_t out_val;
    cr_assert(hm_get_value(hm, 10, &out_val), "Probe chain must survive tombstones");
    cr_assert_eq(out_val, 300);

    // Ensure we can re-insert into the map (potentially reusing the tombstone)
    cr_assert(hm_insert_new(hm, 15, 400));
    cr_assert(hm_get_value(hm, 15, &out_val));
    cr_assert_eq(out_val, 400);

    hm_destroy(hm);
}

Test(hashmap, should_grow_automatically_when_load_factor_exceeded) {
    // Start with a small capacity
    uint32_t initial_capacity = 4;
    hashmap *hm = hm_create(initial_capacity);
    
    // Insert enough elements to force the map to grow
    // > 70% of 4 is 3. So inserting 10 elements guarantees growth.
    for (uint32_t i = 0; i < 10; i++) {
        cr_assert(hm_insert_new(hm, i, i * 10), "Insert failed during heavy load at key %d", i);
    }
    
    cr_assert(hm->capacity > initial_capacity, "Capacity should have grown");
    cr_assert_eq(hm->count, 10, "Count should be exactly 10");

    // Verify all data survived the re-hashing
    uint32_t out_val;
    for (uint32_t i = 0; i < 10; i++) {
        cr_assert(hm_get_value(hm, i, &out_val));
        cr_assert_eq(out_val, i * 10, "Value corrupted during rehash for key %d", i);
    }

    hm_destroy(hm);
}

Test(hashmap, should_handle_null_arguments_safely) {
    uint32_t out_val;

    cr_assert_not(hm_insert_new(NULL, 1, 100));
    cr_assert_not(hm_update_value(NULL, 1, 200));
    cr_assert_not(hm_contains_key(NULL, 1));
    cr_assert_not(hm_get_value(NULL, 1, &out_val));
    cr_assert_not(hm_remove(NULL, 1));
    cr_assert_not(hm_destroy(NULL));

    hashmap *hm = hm_create(5);
    cr_assert_not(hm_get_value(hm, 1, NULL), "Should reject NULL out_value pointer");
    hm_destroy(hm);
}

Test(hashmap, should_reject_invalid_capacity) {
    hashmap *hm = hm_create(0);
    cr_assert_null(hm, "hm_create should return NULL for capacity 0");
}
