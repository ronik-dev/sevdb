#import "internal.h"
#import <stdlib.h>
#import <string.h>


sevdb_vector* sevdb_vector_create(int id, int dimensions ,const float* components){
    size_t components_size = sizeof(float)*dimensions;
    size_t total_size = sizeof(sevdb_vector) + components_size;
    sevdb_vector *v = malloc(total_size);
    if(v == NULL) return NULL;
    v->id = id; 
    v->dimensions = dimensions; 
    memcpy(v->components, components, components_size);
    return v;
}

void sevdb_vector_destroy(sevdb_vector *v){
    if(v != NULL) free(v);
}

