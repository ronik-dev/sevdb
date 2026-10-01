#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "sevdb/sevdb.h"
#include "../src/distance.h" 

int main() {
    // Seed the random number generator
    srand((unsigned int)time(NULL));

    int vector_number = 100000; 
    int k = 5;
    int vector_dimension = 1536; // Standard modern text embedding size

    printf("Generating %d vectors at %d dimensions...\n", vector_number, vector_dimension);
    sevdb_database *db = sevdb_db_create(vector_number);
    if(db == NULL) { printf("Failed to create DB\n"); return 1; }

    // Allocate reusable buffers on the heap instead of the stack
    float *input_components = malloc(vector_dimension * sizeof(float));
    float *search_components = calloc(vector_dimension, sizeof(float));
    
    if(input_components == NULL || search_components == NULL) {
        printf("Failed to allocate component buffers\n");
        return 1;
    }

    // Set up the search vector with random values between -1.0 and 1.0
    for(int d = 0; d < vector_dimension; d++) {
        search_components[d] = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    }
    sevdb_vector *vector_to_compare = sevdb_vector_create(999999, vector_dimension, search_components);

    for (int i = 0; i < vector_number; i++) {
        // Populate all dimensions with random values between -1.0 and 1.0
        for(int d = 0; d < vector_dimension; d++) {
            input_components[d] = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        }
        
        sevdb_vector *v = sevdb_vector_create(i, vector_dimension, input_components);
        
        if(v == NULL || sevdb_db_push_vector(db, v) == NULL) {
            printf("Failed to push vector %d\n", i); 
            return 1; 
        }
    }

    sevdb_vector *out_vector_list[5] = {NULL};

    printf("Starting search...\n");
    clock_t start = clock(); 
    
    int vectors_found = sevdb_db_search_k_similar_vectors(
        db, vector_to_compare, k, out_vector_list
    );
    
    clock_t end = clock();
    printf("Stopped timer, executing sanity checks... \n");

    if(vectors_found != k) {
        printf("Failed: Only found %d vectors, was looking for %d\n", vectors_found, k);
        return 1;
    }

    // Verify sorting order
    for (int i = 0; i < k - 1; i++) {
        float sim_current = get_cosine_similarity(vector_dimension,
            sevdb_vector_get_components(vector_to_compare),
            sevdb_vector_get_components(out_vector_list[i]));

        float sim_next = get_cosine_similarity(vector_dimension,
            sevdb_vector_get_components(vector_to_compare),
            sevdb_vector_get_components(out_vector_list[i + 1]));

        if(sim_current < sim_next) {
            printf("Failed: Vectors are out of order!\n"); 
            return 1; 
        }
    }

    sevdb_db_serialize(db, "./benchmark.bin");
    
    // Clean up heap allocations
    free(input_components);
    free(search_components);
    sevdb_vector_destroy(vector_to_compare);
    sevdb_db_destroy(db);

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("SUCCESS! Search execution time: %f seconds\n", time_spent);
    return 0;
}
