#include <criterion/criterion.h>
#include "../src/internal.h"

Test(vector, create_and_read) {
    float input_components[2] = {4.2f, 5.2f};
    
    sevdb_vector *v = sevdb_vector_create(42, 2, input_components);
    
    cr_assert_not_null(v, "Vector allocation failed");
    cr_assert_eq(v->id, 42); 
    cr_assert_eq(v->dimensions, 2); 
    cr_assert_float_eq(v->components[0], 4.2f, 0.0001); 
    cr_assert_float_eq(v->components[1], 5.2f, 0.0001); 
    
    //clean
    sevdb_vector_destroy(v);
}

Test(database, create_and_push) {

    //create
    sevdb_database *db = sevdb_db_create(42);
    cr_assert_not_null(db,"Database allocation failed");
    cr_assert_eq(db->count,0);
    cr_assert_eq(db->capacity,42);

    //push
    float input_components[2] = {4.2f, 5.2f};
    sevdb_vector *v = sevdb_vector_create(42, 2, input_components);
    sevdb_vector *result = sevdb_db_push_vector(db, v);

    cr_assert_not_null(result, "Push returned NULL");
    cr_assert_eq(db->count, 1, "Database count did not increment");
    cr_assert_eq(db->vectors[0]->id, 42, "Vector ID mismatch in database");

    //clean
    sevdb_db_destroy(db);
}

Test(database, create_push_and_pull) {
    //create
    sevdb_database *db = sevdb_db_create(42);

    //push
    float input_components[2] = {4.2f, 5.2f};
    sevdb_vector *v = sevdb_vector_create(42, 2, input_components);
    sevdb_vector *result = sevdb_db_push_vector(db, v);

    //pull
    sevdb_vector *search = sevdb_db_poll_vector_by_id(db, 42);
    cr_assert(v == search, "Search returned wrong memory address");

    //clean
    sevdb_db_destroy(db);
}

