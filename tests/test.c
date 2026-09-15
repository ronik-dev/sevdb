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
    
    sevdb_vector_destroy(v);
}
