#include <criterion/criterion.h>
#include "../src/distance.h"

//--------------------
//-EUCLIDEAN-DISTANCE-
//--------------------

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
  
//-------------------
//-COSINE-SIMILARITY-
//-------------------
  
// Case 1: Identical direction (Vectors point exactly the same way)
Test(cosine_similarity, identical_direction) {
    // Note: They don't have to be the exact same points, just the same direction
    float v1[2] = {1.0f, 2.0f};
    float v2[2] = {2.0f, 4.0f}; // Scaled by 2
    
    float sim = get_cosine_similarity(2, v1, v2);
    
    cr_assert_float_eq(sim, 1.0f, 0.0001, "Same direction should yield a similarity of 1.0");
}

// Case 2: Orthogonal vectors (90 degrees apart)
Test(cosine_similarity, orthogonal_vectors) {
    float v1[2] = {1.0f, 0.0f};
    float v2[2] = {0.0f, 1.0f};
    
    float sim = get_cosine_similarity(2, v1, v2);
    
    cr_assert_float_eq(sim, 0.0f, 0.0001, "Orthogonal vectors should yield a similarity of 0.0");
}

// Case 3: Opposite direction (180 degrees apart)
Test(cosine_similarity, opposite_direction) {
    float v1[2] = {1.0f, -2.0f};
    float v2[2] = {-1.0f, 2.0f};
    
    float sim = get_cosine_similarity(2, v1, v2);
    
    cr_assert_float_eq(sim, -1.0f, 0.0001, "Opposite directions should yield a similarity of -1.0");
}

// Case 4: Fractional similarity (45 degree angle)
Test(cosine_similarity, fractional_similarity) {
    float v1[3] = {1.0f, 1.0f, 0.0f};
    float v2[3] = {0.0f, 1.0f, 0.0f};
    
    float sim = get_cosine_similarity(3, v1, v2);
    
    // Dot product = 1.0
    // Mag 1 = sqrt(2), Mag 2 = 1
    // Cosine = 1 / sqrt(2) = ~0.7071
    cr_assert_float_eq(sim, 0.7071f, 0.001, "Expected similarity is ~0.7071");
}

// Case 5: edge case (all 0 vector)
Test(cosine_similarity, all_zero_vector) {
    float v1[3] = {1.0f, 1.0f, 0.0f};
    float v2[3] = {0.0f, 0.0f, 0.0f};
    
    float sim = get_cosine_similarity(3, v1, v2);
    
    cr_assert_float_eq(sim, 0, 0.001, "Expected similarity is ~0.0");
}
