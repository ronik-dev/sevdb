#include "sevdb/sevdb.h"
#include "sevdb_private.h"
#include "distance.h"
#include "pqueue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>

//----------
//--VECTOR--
//----------

sevdb_state sevdb_vector_create(uint32_t id, uint32_t dimensions, const float *components, sevdb_vector** result){
    if (result == NULL) return SEVDB_ERROR_INVALID_ARGUMENT;
    *result = NULL;
    if (dimensions == 0 
        || components == NULL 
        || dimensions > SEVDB_MAX_VECTOR_DIMENSIONS) {
        return SEVDB_ERROR_INVALID_ARGUMENT;
    }
    size_t components_size = sizeof(float)*dimensions;
    size_t total_size = sizeof(sevdb_vector) + components_size;
    sevdb_vector *v = malloc(total_size);
    if (v == NULL) return SEVDB_ERROR_OUT_OF_MEMORY;
    v->id = id; 
    v->dimensions = dimensions; 
    memcpy(v->components, components, components_size);
    *result = v; 
    return SEVDB_OK;
}

void sevdb_vector_destroy(sevdb_vector *v){
    if (v != NULL) free(v);
}

uint32_t sevdb_vector_get_id(const sevdb_vector *v){
    assert(v != NULL);
    return v->id;
}

uint32_t sevdb_vector_get_dimensions(const sevdb_vector *v){
    assert(v != NULL);
    return v->dimensions;
}

const float* sevdb_vector_get_components(const sevdb_vector *v){
    assert(v != NULL);
    return v->components;
}



//------------
//--DATABASE--
//------------

sevdb_state sevdb_db_create(uint32_t capacity, sevdb_database **result){
    if (capacity == 0 || result == NULL) return SEVDB_ERROR_INVALID_ARGUMENT;
    *result = NULL;
    sevdb_database *db = malloc(sizeof(sevdb_database));
    if (db == NULL) return SEVDB_ERROR_OUT_OF_MEMORY;
    db->capacity = capacity;
    db->count = 0;
    db->vectors = calloc(capacity, sizeof(sevdb_vector*));
    if (db->vectors == NULL){
        free(db);
        return SEVDB_ERROR_OUT_OF_MEMORY;
    }
    hashmap* hm = NULL;
    switch(hm_create(capacity,&hm)){
        case HM_OK:
              break;
        case HM_ERROR_OUT_OF_MEMORY:
            free(db);
            return SEVDB_ERROR_OUT_OF_MEMORY;
        default:
            free(db);
            return SEVDB_ERROR_INTERNAL;
    }
    db->vector_id_map = hm;
    if (db->vector_id_map == NULL){
        free(db->vectors);
        free(db);
        return SEVDB_ERROR_INTERNAL;
    }
    *result = db;
    return SEVDB_OK;
}

void sevdb_db_destroy(sevdb_database *db){
    if (db == NULL) return; 
    if (db->vectors != NULL){
        for (uint32_t i = 0; i < db->capacity; i++){
            if (db->vectors[i] != NULL){
                sevdb_vector_destroy(db->vectors[i]);  
            }
        }
        free(db->vectors);
    }
    if (db->vector_id_map != NULL) hm_destroy(db->vector_id_map);
    free(db);
}


sevdb_state sevdb_db_increase_capacity(sevdb_database *db, uint32_t increase){
    if (db == NULL || increase == 0) return SEVDB_ERROR_INVALID_ARGUMENT;
    uint64_t new_capacity = (uint64_t)db->capacity + increase;
    if (new_capacity > UINT32_MAX) return SEVDB_ERROR_INVALID_ARGUMENT;

    if ((size_t)new_capacity > SIZE_MAX / sizeof(*db->vectors)) {
        return SEVDB_ERROR_OVERFLOW;
    }
    size_t new_size = new_capacity * sizeof(sevdb_vector*);
    sevdb_vector** temp = realloc(db->vectors, new_size);
    if (temp == NULL) return SEVDB_ERROR_OUT_OF_MEMORY;
    memset(temp + db->capacity, 0,(new_capacity - db->capacity) * sizeof(sevdb_vector*));

    db->capacity = (uint32_t)new_capacity;
    db->vectors = temp; 
    return SEVDB_OK;
}

uint32_t sevdb_db_get_capacity(const sevdb_database *db){
    assert(db != NULL);
    return db->capacity;
}

uint32_t sevdb_db_get_count(const sevdb_database *db){
    assert(db != NULL);
    return db->count;
}

sevdb_state sevdb_db_push_vector(sevdb_database *db, sevdb_vector *v){
    if (db == NULL || v == NULL) return SEVDB_ERROR_INVALID_ARGUMENT;
    if (db->count >= db->capacity) return SEVDB_ERROR_DATABASE_FULL;
    if (hm_contains_key(db->vector_id_map, v->id)) {
        return SEVDB_ERROR_DUPLICATE_ID;
    }
    uint32_t target_index = db->count;
    switch(hm_insert_new(db->vector_id_map, v->id, target_index)){
        case HM_OK:
            break;
        case HM_ERROR_DUPLICATE_KEY:
            return SEVDB_ERROR_DUPLICATE_ID;
        case HM_ERROR_OUT_OF_MEMORY:
            return SEVDB_ERROR_OUT_OF_MEMORY;
        case HM_ERROR_INVALID_ARGUMENT:
        default:
            return SEVDB_ERROR_INTERNAL;
    }
    db->vectors[target_index] = v;
    db->count++;
    return SEVDB_OK;
}

sevdb_vector* sevdb_db_get_vector_by_id(sevdb_database *db, uint32_t id){
    if (db == NULL || db->count == 0) return NULL;
    uint32_t index; 
    switch(hm_get_value(db->vector_id_map, id, &index)){
        case HM_OK:
            break;
        case HM_ERROR_NOT_FOUND:
        case HM_ERROR_INVALID_ARGUMENT:
        default:
            return NULL;
    }
    if (index >= db->count) return NULL;
    sevdb_vector *v = db->vectors[index];
    if (v != NULL && v->id == id){
        return v; 
    }
    return NULL;
}

sevdb_state sevdb_db_remove_vector_by_id(sevdb_database *db, uint32_t id){
    if (db == NULL) return SEVDB_ERROR_INVALID_ARGUMENT;
    if (db->count == 0) return SEVDB_ERROR_NOT_FOUND;
    uint32_t index;
    
    switch(hm_get_value(db->vector_id_map, id, &index)){
        case HM_OK:
            break;
        case HM_ERROR_NOT_FOUND:
            return SEVDB_ERROR_NOT_FOUND;
        case HM_ERROR_INVALID_ARGUMENT:
        default:
            return SEVDB_ERROR_INTERNAL;
    }
    
    // Validate the map entry before modifying anything.
    if (index >= db->count ||
        db->vectors[index] == NULL ||
        db->vectors[index]->id != id) {
        return SEVDB_ERROR_INTERNAL;
    }

    uint32_t last_index = db->count - 1;
    sevdb_vector *removed = db->vectors[index];
    
    if (index != last_index) {
        sevdb_vector *last_vector = db->vectors[last_index];
        if (last_vector == NULL) {
            return SEVDB_ERROR_INTERNAL;
        }
        
        // Update the last vector's index before modifying the array.
        switch(hm_update_value(db->vector_id_map, last_vector->id, index)) {
            case HM_OK:
                break;
            case HM_ERROR_INVALID_ARGUMENT:
            default:
                return SEVDB_ERROR_INTERNAL;
        }
        
        // Remove the target ID. If this fails, roll back the index.
        switch(hm_remove(db->vector_id_map, id)) {
            case HM_OK:
                break;
            default:
                hm_state rollback_ok = hm_update_value(db->vector_id_map, last_vector->id, last_index);
                // Failure here indicates a broken internal invariant.
                assert(rollback_ok == HM_OK); 
                (void)rollback_ok;
                return SEVDB_ERROR_INTERNAL;
        }
        db->vectors[index] = last_vector;
    } else {
        if (hm_remove(db->vector_id_map, id) != HM_OK) {
            return SEVDB_ERROR_INTERNAL;
        }
    }
    
    db->vectors[last_index] = NULL;
    db->count--;
    sevdb_vector_destroy(removed);
    
    return SEVDB_OK;
}

static bool is_max_heap(float a, float b) { return a > b; }
static bool is_min_heap(float a, float b) { return a < b; }

sevdb_state sevdb_db_search_k_similar_vectors(sevdb_database *db, const sevdb_vector* v, int k, int* out_result_count, sevdb_similarity_scored_vector* out_results){
    if (db == NULL || v == NULL || out_results == NULL || out_result_count == NULL) return SEVDB_ERROR_INVALID_ARGUMENT;
    if (k<=0) return SEVDB_ERROR_INVALID_ARGUMENT;
    if (db->count == 0){
        *out_result_count = 0;
        return SEVDB_OK;
    }

    pqueue* pq;
    switch(pq_create(k, is_min_heap, &pq)){
        case PQ_OK:
            break;
        case PQ_ERROR_OUT_OF_MEMORY:
            return SEVDB_ERROR_OUT_OF_MEMORY;
        default:
            return SEVDB_ERROR_INTERNAL;
            break;
    }
    if (pq == NULL) return SEVDB_ERROR_INTERNAL;

    sevdb_vector* candidate;
    pq_element worst_saved_el;
    float candidate_cos_sim;

    int pq_capacity;
    int pq_count;

    for(uint32_t i = 0; i < db->count; i++){
        candidate = db->vectors[i];
        if (candidate == NULL || candidate->dimensions != v->dimensions) {
            continue; 
        }

        candidate_cos_sim = get_cosine_similarity(v->dimensions, v->components, candidate->components);

        if(!pq_get_capacity(pq, &pq_capacity)) return SEVDB_ERROR_INTERNAL;
        if(!pq_get_count(pq, &pq_count)) return SEVDB_ERROR_INTERNAL;

        if (pq_count < pq_capacity){
            switch (pq_enqueue(pq, candidate_cos_sim, candidate)){
                case PQ_OK:
                    break;
                default:
                    pq_destroy(pq);
                    return SEVDB_ERROR_INTERNAL;
            }
        } else {
            if (!pq_peek(pq, &worst_saved_el)){
                pq_destroy(pq);
                return SEVDB_ERROR_INTERNAL; 
            }

            if (candidate_cos_sim > worst_saved_el.priority){
                if (!(pq_dequeue(pq, &worst_saved_el) && pq_enqueue(pq, candidate_cos_sim, candidate) != PQ_OK)){
                    pq_destroy(pq);
                    return SEVDB_ERROR_INTERNAL;
                }
            }
        }
    }

    int retrieved;
    if(!pq_get_count(pq, &retrieved)){
        return SEVDB_ERROR_INTERNAL;
    }
    pq_element out;

    for(int i = retrieved - 1; i >= 0; i--){
        if  (pq_dequeue(pq, &out)) {
            out_results[i].vector = (sevdb_vector*)out.content;
            out_results[i].score = out.priority;
        }
    }

    pq_destroy(pq);
    *out_result_count = retrieved; 
    return SEVDB_OK;
}
