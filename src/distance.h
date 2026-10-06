#pragma once
#include <math.h>

/**
 * @brief Calculates the Euclidean (Pythagorean) distance between two vectors.
 * 
 * Computes the straight-line length between two points in Euclidean space.
 * 
 * @note **Ordering:** Smaller values indicate vectors are closer together (0.0 is an exact match).
 * 
 * @param dimensions The number of components in the vectors. Must be > 0.
 * @param v1 Pointer to the first vector's component array.
 * @param v2 Pointer to the second vector's component array.
 * @return The calculated distance as a float. 
 */
float get_euclidean_distance(int dimensions, const float* v1, const float* v2);

/**
 * @brief Calculates the Cosine Similarity between two non-zero vectors.
 * 
 * Measures the cosine of the angle between two vectors (their dot product divided 
 * by the product of their magnitudes). This metric ignores the length of the vectors 
 * and strictly compares their direction.
 * 
 * @note **Ordering:** Larger values indicate vectors are closer together. 
 *       The result strictly falls in the interval [-1.0, +1.0], where 1.0 is an 
 *       exact directional match, 0.0 is orthogonal, and -1.0 is diametrically opposed.
 * @warning If either vector is a zero-vector (all components are 0.0), this calculation 
 *          will result in a division by zero, the return value in this case is 0.
 * 
 * @param dimensions The number of components in the vectors. Must be > 0.
 * @param v1 Pointer to the first vector's component array.
 * @param v2 Pointer to the second vector's component array.
 * @return The cosine similarity score as a float.
 */
float get_cosine_similarity(int dimensions, const float* v1, const float* v2);
