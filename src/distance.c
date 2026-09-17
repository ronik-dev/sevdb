#include "distance.h"
#include <math.h>
#include <string.h>

float get_euclidean_distance(int dimensions, float* v1, float* v2){
    if (dimensions <= 0) return 0;
    if (v1 == NULL | v2 == NULL) return 0;
    float diff;
    float squared_sums;
    for(int i = 0; i < dimensions; i++){
        diff = v1[i]-v2[i];
        squared_sums += diff * diff;
    }
    return sqrtf(squared_sums);
}

float get_cosine_similarity(int dimensions, float* v1, float* v2){
    if (dimensions <= 0) return 0;
    if (v1 == NULL | v2 == NULL) return 0;
    float v1v2;
    float v1_squared;
    float v2_squared;
    for(int i = 0; i < dimensions; i++) {
        v1v2 += v1[i] * v2[i];
        v1_squared += v1[i] * v1[i];
        v2_squared += v2[i] * v2[i];
    }
    if(v1_squared == 0 || v2_squared == 0) return 0;
    return v1v2 / (sqrtf(v1_squared) * sqrtf(v2_squared));
}
