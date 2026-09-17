#include <criterion/criterion.h>
#include "../src/distance.h"

// Case 1: Distance between the exact same points must be 0
Test(euclidean_distance, identical_vectors) {
    float v1[2] = {1.5f, 2.5f};
    float v2[2] = {1.5f, 2.5f};
    
    float dist = get_euclidean_distance(2, v1, v2);
    
    cr_assert_float_eq(dist, 0.0f, 0.0001, "Distance between identical vectors should be 0");
}

// Case 2: Classic 2D geometry (3-4-5 right triangle)
Test(euclidean_distance, standard_2d_points) {
    float v1[2] = {0.0f, 0.0f};
    float v2[2] = {3.0f, 4.0f};
    
    float dist = get_euclidean_distance(2, v1, v2);
    
    // sqrt((0-3)^2 + (0-4)^2) = sqrt(9 + 16) = 5
    cr_assert_float_eq(dist, 5.0f, 0.0001, "Expected distance is 5.0");
}

// Case 3: 3D coordinates handling negative numbers
Test(euclidean_distance, 3d_points_with_negatives) {
    float v1[3] = {1.0f, -2.0f, 3.0f};
    float v2[3] = {4.0f, 2.0f, 3.0f};
    
    float dist = get_euclidean_distance(3, v1, v2);
    
    // sqrt((1-4)^2 + (-2-2)^2 + (3-3)^2) = sqrt(9 + 16 + 0) = 5
    cr_assert_float_eq(dist, 5.0f, 0.0001, "Expected distance is 5.0");
}

// Case 4: Non-integer fractional distances
Test(euclidean_distance, fractional_result) {
    float v1[2] = {-1.0f, -1.0f};
    float v2[2] = {1.0f, 1.0f};
    
    float dist = get_euclidean_distance(2, v1, v2);
    
    // sqrt((-1-1)^2 + (-1-1)^2) = sqrt(8) ~ 2.828427
    cr_assert_float_eq(dist, 2.8284f, 0.001, "Expected distance is ~2.8284");
}
