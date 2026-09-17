#include <math.h>

// In mathematics, the Euclidean distance between two points 
// in a Euclidean space is the length of the line segment between them. 
// It can be calculated from the Cartesian coordinates of the points 
// using the Pythagorean theorem, and therefore is occasionally 
// called the Pythagorean distance
float get_euclidean_distance(int dimensions, float* v1, float* v2);

// In data analysis, cosine similarity is a measure of similarity 
// between two non-zero vectors defined in an inner product space. 
// Cosine similarity is the cosine of the angle between the vectors; 
// that is, it is the dot product of the vectors divided by the product of their lengths. 
// It follows that the cosine similarity does not depend 
// on the magnitudes of the vectors, but only on their angle. 
// The cosine similarity always belongs to the interval [-1,+1].
float get_cosine_similarity(int dimensions, float* v1, float* v2);
