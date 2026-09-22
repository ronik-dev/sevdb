#include <criterion/criterion.h>
#include <stdio.h>
#include "sevdb/sevdb.h"
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

Test(database, should_serialize_and_deserialize_with_vectors) {
    const char *path = "./test_serialize_vectors.bin";

    sevdb_database *db = sevdb_db_create(42);
    cr_assert_not_null(db);

    /* Populate database with vectors having different dimensions/values. */
    float components_1[2] = {4.2f, 5.2f};
    float components_2[2] = {1.0f, 2.0f};
    float components_3[2] = {-3.5f, 7.25f};

    sevdb_vector *v1 = sevdb_vector_create(10, 2, components_1);
    sevdb_vector *v2 = sevdb_vector_create(20, 2, components_2);
    sevdb_vector *v3 = sevdb_vector_create(30, 2, components_3);

    cr_assert_not_null(v1);
    cr_assert_not_null(v2);
    cr_assert_not_null(v3);

    cr_assert_not_null(sevdb_db_push_vector(db, v1));
    cr_assert_not_null(sevdb_db_push_vector(db, v2));
    cr_assert_not_null(sevdb_db_push_vector(db, v3));

    cr_assert_eq(sevdb_database_get_count(db), 3);

    /* Serialize. */
    cr_assert(
        sevdb_db_serialize(db, path) == true,
        "Failed to serialize populated database"
    );

    /* Deserialize. */
    sevdb_database *loaded_db = sevdb_db_deserialize(path);

    cr_assert_not_null(
        loaded_db,
        "Failed to deserialize populated database"
    );

    /* Database metadata. */
    cr_assert_eq(
        sevdb_database_get_capacity(loaded_db),
        42,
        "Deserialized database has wrong capacity"
    );

    cr_assert_eq(
        sevdb_database_get_count(loaded_db),
        3,
        "Deserialized database has wrong vector count"
    );

    /* Verify vector 1. */
    sevdb_vector *loaded_v1 =
        sevdb_db_get_vector_by_id(loaded_db, 10);

    cr_assert_not_null(loaded_v1);
    cr_assert_eq(sevdb_vector_get_id(loaded_v1), 10);
    cr_assert_eq(sevdb_vector_get_dimensions(loaded_v1), 2);

    const float *loaded_components_1 =
        sevdb_vector_get_components(loaded_v1);

    cr_assert_float_eq(loaded_components_1[0], 4.2f, 0.0001);
    cr_assert_float_eq(loaded_components_1[1], 5.2f, 0.0001);

    /* Verify vector 2. */
    sevdb_vector *loaded_v2 =
        sevdb_db_get_vector_by_id(loaded_db, 20);

    cr_assert_not_null(loaded_v2);
    cr_assert_eq(sevdb_vector_get_id(loaded_v2), 20);
    cr_assert_eq(sevdb_vector_get_dimensions(loaded_v2), 2);

    const float *loaded_components_2 =
        sevdb_vector_get_components(loaded_v2);

    cr_assert_float_eq(loaded_components_2[0], 1.0f, 0.0001);
    cr_assert_float_eq(loaded_components_2[1], 2.0f, 0.0001);

    /* Verify vector 3, including negative/decimal values. */
    sevdb_vector *loaded_v3 =
        sevdb_db_get_vector_by_id(loaded_db, 30);

    cr_assert_not_null(loaded_v3);
    cr_assert_eq(sevdb_vector_get_id(loaded_v3), 30);
    cr_assert_eq(sevdb_vector_get_dimensions(loaded_v3), 2);

    const float *loaded_components_3 =
        sevdb_vector_get_components(loaded_v3);

    cr_assert_float_eq(loaded_components_3[0], -3.5f, 0.0001);
    cr_assert_float_eq(loaded_components_3[1], 7.25f, 0.0001);

    /* Cleanup. */
    remove(path);

    sevdb_db_destroy(db);
    sevdb_db_destroy(loaded_db);
}


Test(database, should_preserve_search_results_after_deserialization) {
    const char *path = "./test_search_deserialization.bin";

    sevdb_database *db = sevdb_db_create(10);
    cr_assert_not_null(db);

    /*
     * Vectors are deliberately chosen so their cosine similarities
     * relative to the query are different and deterministic.
     */
    float c0[2] = {1.0f, 0.0f};
    float c1[2] = {0.9f, 0.1f};
    float c2[2] = {0.0f, 1.0f};
    float c3[2] = {-1.0f, 0.0f};

    sevdb_vector *v0 = sevdb_vector_create(0, 2, c0);
    sevdb_vector *v1 = sevdb_vector_create(1, 2, c1);
    sevdb_vector *v2 = sevdb_vector_create(2, 2, c2);
    sevdb_vector *v3 = sevdb_vector_create(3, 2, c3);

    cr_assert_not_null(sevdb_db_push_vector(db, v0));
    cr_assert_not_null(sevdb_db_push_vector(db, v1));
    cr_assert_not_null(sevdb_db_push_vector(db, v2));
    cr_assert_not_null(sevdb_db_push_vector(db, v3));

    cr_assert(sevdb_db_serialize(db, path));

    sevdb_database *loaded_db = sevdb_db_deserialize(path);
    cr_assert_not_null(loaded_db);

    float query_components[2] = {1.0f, 0.0f};

    sevdb_vector *query =
        sevdb_vector_create(99, 2, query_components);

    cr_assert_not_null(query);

    sevdb_vector *results_original[4] = {0};
    sevdb_vector *results_loaded[4] = {0};

    int original_count =
        sevdb_db_search_k_similar_vectors(
            db,
            query,
            4,
            results_original
        );

    int loaded_count =
        sevdb_db_search_k_similar_vectors(
            loaded_db,
            query,
            4,
            results_loaded
        );

    cr_assert_eq(original_count, loaded_count);

    /*
     * The important part here is that serialization did not change
     * the searchable contents of the database.
     */
    for (int i = 0; i < original_count; i++) {
        cr_assert_not_null(results_original[i]);
        cr_assert_not_null(results_loaded[i]);

        cr_assert_eq(
            sevdb_vector_get_id(results_original[i]),
            sevdb_vector_get_id(results_loaded[i]),
            "Search result %d changed after deserialization",
            i
        );

        cr_assert_eq(
            sevdb_vector_get_dimensions(results_original[i]),
            sevdb_vector_get_dimensions(results_loaded[i])
        );
    }

    sevdb_vector_destroy(query);
    sevdb_db_destroy(db);
    sevdb_db_destroy(loaded_db);

    remove(path);
}


Test(database, should_preserve_vector_count_after_deserialization) {
    const char *path = "./test_vector_count.bin";

    sevdb_database *db = sevdb_db_create(100);

    for (int i = 0; i < 20; i++) {
        float components[3] = {
            (float)i,
            (float)i + 0.5f,
            (float)i * -2.0f
        };

        sevdb_vector *v =
            sevdb_vector_create(i, 3, components);

        cr_assert_not_null(v);
        cr_assert_not_null(sevdb_db_push_vector(db, v));
    }

    cr_assert_eq(sevdb_database_get_count(db), 20);

    cr_assert(sevdb_db_serialize(db, path));

    sevdb_database *loaded_db =
        sevdb_db_deserialize(path);

    cr_assert_not_null(loaded_db);

    cr_assert_eq(
        sevdb_database_get_capacity(loaded_db),
        100
    );

    cr_assert_eq(
        sevdb_database_get_count(loaded_db),
        20
    );

    /* Verify every vector can still be found. */
    for (int i = 0; i < 20; i++) {
        sevdb_vector *v =
            sevdb_db_get_vector_by_id(loaded_db, i);

        cr_assert_not_null(
            v,
            "Vector with id %d was lost during serialization",
            i
        );

        cr_assert_eq(sevdb_vector_get_id(v), i);
        cr_assert_eq(sevdb_vector_get_dimensions(v), 3);

        const float *components =
            sevdb_vector_get_components(v);

        cr_assert_float_eq(
            components[0],
            (float)i,
            0.0001
        );

        cr_assert_float_eq(
            components[1],
            (float)i + 0.5f,
            0.0001
        );

        cr_assert_float_eq(
            components[2],
            (float)i * -2.0f,
            0.0001
        );
    }

    sevdb_db_destroy(db);
    sevdb_db_destroy(loaded_db);

    remove(path);
}


Test(database, should_serialize_and_deserialize_empty_database) {
    const char *path = "./test_empty_database.bin";

    sevdb_database *db = sevdb_db_create(42);

    cr_assert_not_null(db);
    cr_assert_eq(sevdb_database_get_count(db), 0);

    cr_assert(
        sevdb_db_serialize(db, path),
        "Failed to serialize empty database"
    );

    sevdb_database *loaded_db =
        sevdb_db_deserialize(path);

    cr_assert_not_null(
        loaded_db,
        "Failed to deserialize empty database"
    );

    cr_assert_eq(
        sevdb_database_get_capacity(loaded_db),
        42
    );

    cr_assert_eq(
        sevdb_database_get_count(loaded_db),
        0
    );

    sevdb_db_destroy(db);
    sevdb_db_destroy(loaded_db);

    remove(path);
}


Test(database, should_preserve_high_dimension_vectors) {
    const char *path = "./test_high_dimension_db.bin";

    const int dimensions = 128;

    sevdb_database *db = sevdb_db_create(2);

    float components[dimensions];

    for (int i = 0; i < dimensions; i++) {
        components[i] =
            (float)i * 0.12345f - 7.5f;
    }

    sevdb_vector *v =
        sevdb_vector_create(12345, dimensions, components);

    cr_assert_not_null(v);
    cr_assert_not_null(sevdb_db_push_vector(db, v));

    cr_assert(sevdb_db_serialize(db, path));

    sevdb_database *loaded_db =
        sevdb_db_deserialize(path);

    cr_assert_not_null(loaded_db);

    sevdb_vector *loaded =
        sevdb_db_get_vector_by_id(loaded_db, 12345);

    cr_assert_not_null(loaded);

    cr_assert_eq(
        sevdb_vector_get_id(loaded),
        12345
    );

    cr_assert_eq(
        sevdb_vector_get_dimensions(loaded),
        dimensions
    );

    const float *loaded_components =
        sevdb_vector_get_components(loaded);

    for (int i = 0; i < dimensions; i++) {
        cr_assert_float_eq(
            loaded_components[i],
            components[i],
            0.0001,
            "Component %d changed after deserialization",
            i
        );
    }

    sevdb_db_destroy(db);
    sevdb_db_destroy(loaded_db);

    remove(path);
}


Test(database, should_not_find_missing_vector_after_deserialization) {
    const char *path = "./test_missing_vector.bin";

    sevdb_database *db = sevdb_db_create(5);

    float components[2] = {1.0f, 2.0f};

    sevdb_vector *v =
        sevdb_vector_create(123, 2, components);

    cr_assert_not_null(sevdb_db_push_vector(db, v));

    cr_assert(sevdb_db_serialize(db, path));

    sevdb_database *loaded_db =
        sevdb_db_deserialize(path);

    cr_assert_not_null(loaded_db);

    cr_assert_null(
        sevdb_db_get_vector_by_id(loaded_db, 999),
        "Deserialized database returned a vector for a missing ID"
    );

    sevdb_db_destroy(db);
    sevdb_db_destroy(loaded_db);

    remove(path);
}


Test(database, should_preserve_vectors_after_remove_and_deserialization) {
    const char *path = "./test_remove_before_serialize.bin";

    sevdb_database *db = sevdb_db_create(10);

    for (int i = 0; i < 5; i++) {
        float components[2] = {
            (float)i,
            (float)i + 1.0f
        };

        sevdb_vector *v =
            sevdb_vector_create(i, 2, components);

        cr_assert_not_null(sevdb_db_push_vector(db, v));
    }

    cr_assert_eq(sevdb_database_get_count(db), 5);

    /* Remove some vectors before serialization. */
    sevdb_db_remove_vector_by_id(db, 1);
    sevdb_db_remove_vector_by_id(db, 3);

    cr_assert_eq(sevdb_database_get_count(db), 3);

    cr_assert(sevdb_db_serialize(db, path));

    sevdb_database *loaded_db =
        sevdb_db_deserialize(path);

    cr_assert_not_null(loaded_db);

    printf(
        "ORIGINAL: capacity=%d count=%d\n",
        sevdb_database_get_capacity(db),
        sevdb_database_get_count(db)
    );
    
    printf(
        "LOADED: capacity=%d count=%d\n",
        sevdb_database_get_capacity(loaded_db),
        sevdb_database_get_count(loaded_db)
    );


    cr_assert_eq(
        sevdb_database_get_count(loaded_db),
        3
    );

    cr_assert_not_null(
        sevdb_db_get_vector_by_id(loaded_db, 0)
    );

    cr_assert_null(
        sevdb_db_get_vector_by_id(loaded_db, 1)
    );

    cr_assert_not_null(
        sevdb_db_get_vector_by_id(loaded_db, 2)
    );

    cr_assert_null(
        sevdb_db_get_vector_by_id(loaded_db, 3)
    );

    cr_assert_not_null(
        sevdb_db_get_vector_by_id(loaded_db, 4)
    );

    sevdb_db_destroy(db);
    sevdb_db_destroy(loaded_db);

    remove(path);
}
