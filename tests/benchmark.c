#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "sevdb/sevdb.h"
#include "../src/distance.h"

int main(void) {
    srand((unsigned int)time(NULL));

    int vector_number = 100000;
    int k = 5;
    int vector_dimension = 1536;

    printf(
        "Generating %d vectors at %d dimensions...\n",
        vector_number,
        vector_dimension
    );

    sevdb_database *db = sevdb_db_create(vector_number);
    if (db == NULL) {
        printf("Failed to create DB\n");
        return 1;
    }

    /*
     * Allocate reusable buffers on the heap instead of the stack.
     */
    float *input_components =
        malloc((size_t)vector_dimension * sizeof(float));

    float *search_components =
        calloc((size_t)vector_dimension, sizeof(float));

    if (input_components == NULL || search_components == NULL) {
        printf("Failed to allocate component buffers\n");
        free(input_components);
        free(search_components);
        sevdb_db_destroy(db);
        return 1;
    }

    /*
     * Set up the search vector with random values between -1.0 and 1.0.
     */
    for (int d = 0; d < vector_dimension; d++) {
        search_components[d] =
            ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    }

    sevdb_vector *vector_to_compare =
        sevdb_vector_create(
            999999,
            vector_dimension,
            search_components
        );

    if (vector_to_compare == NULL) {
        printf("Failed to create search vector\n");
        free(input_components);
        free(search_components);
        sevdb_db_destroy(db);
        return 1;
    }

    /*
     * Generate and insert random vectors.
     */
    for (int i = 0; i < vector_number; i++) {
        for (int d = 0; d < vector_dimension; d++) {
            input_components[d] =
                ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        }

        sevdb_vector *v =
            sevdb_vector_create(
                (uint32_t)i,
                vector_dimension,
                input_components
            );

        if (v == NULL || sevdb_db_push_vector(db, v) == NULL) {
            printf("Failed to push vector %d\n", i);

            /*
             * If push failed, v is still owned by the caller.
             * Destroy it only when it was actually created.
             */
            if (v != NULL) {
                sevdb_vector_destroy(v);
            }

            sevdb_vector_destroy(vector_to_compare);
            free(input_components);
            free(search_components);
            sevdb_db_destroy(db);
            return 1;
        }
    }

    sevdb_similarity_scored_vector out_results[5] = {0};

    printf("Starting search...\n");

    clock_t start = clock();

    int vectors_found =
        sevdb_db_search_k_similar_vectors(
            db,
            vector_to_compare,
            k,
            out_results
        );

    clock_t end = clock();

    printf("Stopped timer, executing sanity checks...\n");

    if (vectors_found != k) {
        printf(
            "Failed: Only found %d vectors, was looking for %d\n",
            vectors_found,
            k
        );

        sevdb_vector_destroy(vector_to_compare);
        free(input_components);
        free(search_components);
        sevdb_db_destroy(db);
        return 1;
    }

    /*
     * Verify that results are ordered from highest similarity
     * to lowest similarity.
     */
    for (int i = 0; i < k - 1; i++) {
        if (out_results[i].score < out_results[i + 1].score) {
            printf("Failed: Vectors are out of order!\n");

            sevdb_vector_destroy(vector_to_compare);
            free(input_components);
            free(search_components);
            sevdb_db_destroy(db);
            return 1;
        }
    }

    /*
     * Verify that every returned result has a valid vector.
     */
    for (int i = 0; i < k; i++) {
        if (out_results[i].vector == NULL) {
            printf("Failed: Result %d has a NULL vector!\n", i);

            sevdb_vector_destroy(vector_to_compare);
            free(input_components);
            free(search_components);
            sevdb_db_destroy(db);
            return 1;
        }
    }

    if (!sevdb_db_serialize(db, "./benchmark.bin")) {
        printf("Failed to serialize database\n");

        sevdb_vector_destroy(vector_to_compare);
        free(input_components);
        free(search_components);
        sevdb_db_destroy(db);
        return 1;
    }

    free(input_components);
    free(search_components);
    sevdb_vector_destroy(vector_to_compare);
    sevdb_db_destroy(db);

    double time_spent =
        (double)(end - start) / CLOCKS_PER_SEC;

    printf(
        "SUCCESS! Search execution time: %f seconds\n",
        time_spent
    );

    return 0;
}
