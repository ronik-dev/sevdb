#include <criterion/criterion.h>
#include <stdio.h>
#include "../src/internal.h"
#include "../src/distance.h"

Test(vector, should_create_and_read) {
    float input_components[2] = {4.2f, 5.2f};
    
    sevdb_vector *v = sevdb_vector_create(42, 2, input_components);
    
    cr_assert_not_null(v, "Vector allocation failed");
    cr_assert_eq(sevdb_vector_get_id(v), 42); 
    cr_assert_eq(sevdb_vector_get_dimensions(v), 2); 
    cr_assert_float_eq(sevdb_vector_get_components(v)[0], 4.2f, 0.0001); 
    cr_assert_float_eq(sevdb_vector_get_components(v)[1], 5.2f, 0.0001); 
    
    //clean
    sevdb_vector_destroy(v);
}

Test(database, should_create_push_and_read) {
    //create
    sevdb_database *db = sevdb_db_create(42);

    //push
    float input_components[2] = {4.2f, 5.2f};
    sevdb_vector *v = sevdb_vector_create(42, 2, input_components);
    sevdb_vector *result = sevdb_db_push_vector(db, v);

    //read
    sevdb_vector *search = sevdb_db_get_vector_by_id(db, 42);
    cr_assert(v == search, "Search by id returned wrong memory address");

    //clean
    sevdb_db_destroy(db);
}


Test(database, should_create_push_remove_and_read) {
    //create
    sevdb_database *db = sevdb_db_create(42);

    //push
    float input_components[2] = {4.2f, 5.2f};
    sevdb_vector *v = sevdb_vector_create(42, 2, input_components);
    sevdb_vector *result = sevdb_db_push_vector(db, v);
    int old_number_of_vectors = sevdb_database_get_count(db);

    //remove
    sevdb_db_remove_vector_by_id(db, 42);

    //read
    sevdb_vector *search = sevdb_db_get_vector_by_id(db, 42);
    cr_assert(search == NULL, "Remove by id did not occour, vector is still present");
    cr_assert(old_number_of_vectors -1  == sevdb_database_get_count(db), "DB vector count was not succesfully updated");

    //clean
    sevdb_db_destroy(db);
}


Test(database, should_create_push_and_perform_cosine_similarity_search) {
    int vector_number = 10;
    int k = 5;

    sevdb_database *db = sevdb_db_create(vector_number);
    cr_assert_not_null(db);

    int vector_dimension = 2;

    for (int i = 0; i < vector_number; i++) {
        float input_components[2] = {
            1.0f,
            (float)i * 0.5f
        };

        sevdb_vector *v =
            sevdb_vector_create(i, vector_dimension, input_components);

        cr_assert_not_null(v);
        cr_assert_not_null(sevdb_db_push_vector(db, v));
    }

    float search_components[2] = {1.0f, 0.0f};

    sevdb_vector *vector_to_compare =
        sevdb_vector_create(99, vector_dimension, search_components);

    cr_assert_not_null(vector_to_compare);

    sevdb_vector *out_vector_list[5] = {0};

    int number_of_selected_vectors =
        sevdb_db_search_k_similar_vectors(
            db,
            vector_to_compare,
            k,
            out_vector_list
        );

    cr_assert_eq(number_of_selected_vectors, k,
                 "Expected %d results, got %d",
                 k,
                 number_of_selected_vectors);

    for (int i = 0; i < k; i++) {
        cr_assert_not_null(out_vector_list[i]);
    }

    for (int i = 0; i < k - 1; i++) {
        float sim_current = get_cosine_similarity(
            vector_dimension,
            sevdb_vector_get_components(vector_to_compare),
            sevdb_vector_get_components(out_vector_list[i])
        );

        float sim_next = get_cosine_similarity(
            vector_dimension,
            sevdb_vector_get_components(vector_to_compare),
            sevdb_vector_get_components(out_vector_list[i + 1])
        );

        cr_assert(sim_current >= sim_next,
                  "Cosine search returned wrongly ordered list");
    }

    sevdb_vector_destroy(vector_to_compare);
    sevdb_db_destroy(db);
}


Test(database, should_serialize_and_deserialize) {
    sevdb_database* db = sevdb_db_create(42); 
    const char *path = "./test_db.bin"; 

    cr_assert(sevdb_db_serialize(db, path), "Failed to write magic bytes");
    cr_assert(sevdb_db_deserialize(db, path), "Failed to read magic bytes");

    remove(path); 
    
    sevdb_db_destroy(db); 
}
