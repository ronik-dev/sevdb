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
