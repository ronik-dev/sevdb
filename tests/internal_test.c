#include <criterion/criterion.h>
#include <stdio.h>
#include "sevdb/sevdb.h"
#include "../src/distance.h"

Test(vector, should_create_and_read) {
    float input_components[2] = {4.2f, 5.2f};
    sevdb_vector *v = NULL;
    
    cr_assert_eq(sevdb_vector_create(42, 2, input_components, &v), SEVDB_OK);
    cr_assert_not_null(v, "Vector allocation failed");
    cr_assert_eq(sevdb_vector_get_id(v), 42); 
    cr_assert_eq(sevdb_vector_get_dimensions(v), 2); 
    cr_assert_float_eq(sevdb_vector_get_components(v)[0], 4.2f, 0.0001); 
    cr_assert_float_eq(sevdb_vector_get_components(v)[1], 5.2f, 0.0001); 

    sevdb_vector_destroy(v);
}

Test(vector, should_reject_zero_dimensions) {
    float dummy = 1.0f;
    sevdb_vector *v = (sevdb_vector*)0xDEADBEEF;
    cr_assert_eq(sevdb_vector_create(1, 0, &dummy, &v), SEVDB_ERROR_INVALID_ARGUMENT);
    cr_assert_null(v, "Vector output pointer should be NULL on failure");
}

Test(vector, should_reject_dimensions_over_max) {
    float dummy[1] = {1.0f};
    sevdb_vector *v = (sevdb_vector*)0xDEADBEEF;
    cr_assert_eq(sevdb_vector_create(1, SEVDB_MAX_VECTOR_DIMENSIONS + 1, dummy, &v), SEVDB_ERROR_INVALID_ARGUMENT);
}

Test(database, should_create_push_and_read) {
    sevdb_database *db = NULL;
    cr_assert_eq(sevdb_db_create(42, &db), SEVDB_OK);

    float input_components[2] = {4.2f, 5.2f};
    sevdb_vector *v = NULL;
    sevdb_vector_create(42, 2, input_components, &v);
    
    cr_assert_eq(sevdb_db_push_vector(db, v), SEVDB_OK);

    sevdb_vector *search = sevdb_db_get_vector_by_id(db, 42);
    cr_assert(v == search, "Search by id returned wrong memory address");

    sevdb_db_destroy(db);
}

Test(database, should_reject_duplicate_id_on_push) {
    sevdb_database *db = NULL;
    sevdb_db_create(5, &db);

    float components1[2] = {1.0f, 2.0f};
    sevdb_vector *v1 = NULL;
    sevdb_vector_create(42, 2, components1, &v1);
    cr_assert_eq(sevdb_db_push_vector(db, v1), SEVDB_OK);

    float components2[2] = {9.0f, 9.0f};
    sevdb_vector *v2 = NULL;
    sevdb_vector_create(42, 2, components2, &v2); 
    
    cr_assert_eq(sevdb_db_push_vector(db, v2), SEVDB_ERROR_DUPLICATE_ID);
    cr_assert_eq(sevdb_db_get_count(db), 1, "Count should not increase on rejected push");

    sevdb_vector_destroy(v2); 
    sevdb_db_destroy(db);
}

Test(database, should_create_push_remove_and_read) {
    sevdb_database *db = NULL;
    sevdb_db_create(42, &db);

    float input_components[2] = {4.2f, 5.2f};
    sevdb_vector *v = NULL;
    sevdb_vector_create(42, 2, input_components, &v);
    
    cr_assert_eq(sevdb_db_push_vector(db, v), SEVDB_OK);
    uint32_t old_number_of_vectors = sevdb_db_get_count(db);

    cr_assert_eq(sevdb_db_remove_vector_by_id(db, 42), SEVDB_OK);

    sevdb_vector *search = sevdb_db_get_vector_by_id(db, 42);
    cr_assert(search == NULL, "Remove by id did not occour, vector is still present");
    cr_assert(old_number_of_vectors - 1 == sevdb_db_get_count(db), "DB vector count was not succesfully updated");

    sevdb_db_destroy(db);
}

Test(database, should_increase_capacity) {
    sevdb_database *db = NULL;
    sevdb_db_create(2, &db);

    cr_assert_not_null(db, "Database allocation failed");
    cr_assert_eq(sevdb_db_get_capacity(db), 2);

    float input_components[2] = {4.2f, 5.2f};
    sevdb_vector *v1 = NULL;
    sevdb_vector_create(42, 2, input_components, &v1);
    cr_assert_eq(sevdb_db_push_vector(db, v1), SEVDB_OK);

    cr_assert_eq(sevdb_db_increase_capacity(db, 3), SEVDB_OK);
    cr_assert_eq(sevdb_db_get_capacity(db), 5);

    sevdb_vector *search = sevdb_db_get_vector_by_id(db, 42);
    cr_assert_eq(search, v1, "Vector was lost after increasing capacity");

    float input_components2[2] = {6.2f, 7.2f};
    sevdb_vector *v2 = NULL;
    sevdb_vector_create(43, 2, input_components2, &v2);
    cr_assert_eq(sevdb_db_push_vector(db, v2), SEVDB_OK);

    cr_assert_eq(sevdb_db_get_count(db), 2);

    search = sevdb_db_get_vector_by_id(db, 43);
    cr_assert_eq(search, v2, "Could not retrieve vector added after capacity increase");

    sevdb_db_destroy(db);
}

Test(database, should_handle_multiple_capacity_increases_and_pushes) {
    sevdb_database *db = NULL;
    sevdb_db_create(2, &db);
    cr_assert_not_null(db);

    sevdb_vector *inserted[14] = {0};

    for (int round = 0; round < 3; round++) {
        while (sevdb_db_get_count(db) < sevdb_db_get_capacity(db)) {
            uint32_t idx = sevdb_db_get_count(db);
            float components[2] = {(float)idx, (float)idx * 2.0f};
            sevdb_vector *v = NULL;
            sevdb_vector_create(idx, 2, components, &v);
            
            cr_assert_not_null(v);
            cr_assert_eq(sevdb_db_push_vector(db, v), SEVDB_OK, "Push failed while capacity had room");
            inserted[idx] = v;
        }

        cr_assert_eq(sevdb_db_increase_capacity(db, 4), SEVDB_OK, "Capacity increase failed on round %d", round);
    }

    while (sevdb_db_get_count(db) < sevdb_db_get_capacity(db)) {
        uint32_t idx = sevdb_db_get_count(db);
        float components[2] = {(float)idx, (float)idx * 2.0f};
        sevdb_vector *v = NULL;
        sevdb_vector_create(idx, 2, components, &v);
        
        cr_assert_not_null(v);
        cr_assert_eq(sevdb_db_push_vector(db, v), SEVDB_OK, "Push failed after capacity growth at index %d", idx);
        inserted[idx] = v;
    }

    uint32_t final_count = sevdb_db_get_count(db);
    for (uint32_t i = 0; i < final_count; i++) {
        sevdb_vector *found = sevdb_db_get_vector_by_id(db, i);
        cr_assert_not_null(found, "Vector %d lost after capacity growth", i);
        cr_assert_eq(found, inserted[i], "Vector %d pointer changed unexpectedly after capacity growth", i);
    }

    sevdb_db_destroy(db);
}

Test(database, should_create_push_and_perform_cosine_similarity_search) {
    int vector_number = 10;
    int k = 5;

    sevdb_database *db = NULL;
    sevdb_db_create(vector_number, &db);
    cr_assert_not_null(db);

    int vector_dimension = 2;

    for (int i = 0; i < vector_number; i++) {
        float input_components[2] = {1.0f, (float)i * 0.5f};
        sevdb_vector *v = NULL;
        sevdb_vector_create(i, vector_dimension, input_components, &v);

        cr_assert_not_null(v);
        cr_assert_eq(sevdb_db_push_vector(db, v), SEVDB_OK);
    }

    float search_components[2] = {1.0f, 0.0f};
    sevdb_vector *vector_to_compare = NULL;
    sevdb_vector_create(99, vector_dimension, search_components, &vector_to_compare);

    cr_assert_not_null(vector_to_compare);

    sevdb_similarity_scored_vector out_results[5];
    int number_of_selected_vectors;

    cr_assert_eq(sevdb_db_search_k_similar_vectors(db, vector_to_compare, k, &number_of_selected_vectors, out_results), SEVDB_OK);

    cr_assert_eq(number_of_selected_vectors, k, "Expected %d results, got %d", k, number_of_selected_vectors);

    for (int i = 0; i < k; i++) {
        cr_assert_not_null(out_results[i].vector);
        cr_assert_float_eq(
                out_results[i].score,
                get_cosine_similarity(
                    vector_dimension,
                    sevdb_vector_get_components(vector_to_compare),
                    sevdb_vector_get_components(out_results[i].vector)
                    ),
                0.000001
                );
    }

    for (int i = 0; i < k - 1; i++) {
        cr_assert(out_results[i].score >= out_results[i + 1].score, "Cosine search returned wrongly ordered list");
    }

    sevdb_vector_destroy(vector_to_compare);
    sevdb_db_destroy(db);
}
