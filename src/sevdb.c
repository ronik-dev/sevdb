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

sevdb_vector* sevdb_vector_create(uint32_t id, uint32_t dimensions ,const float* components){
    if (dimensions == 0 || components == NULL || dimensions > SEVDB_MAX_VECTOR_DIMENSIONS) return NULL;
    size_t components_size = sizeof(float)*dimensions;
    size_t total_size = sizeof(sevdb_vector) + components_size;
    sevdb_vector *v = malloc(total_size);
    if (v == NULL) return NULL;
    v->id = id; 
    v->dimensions = dimensions; 
    memcpy(v->components, components, components_size);
    return v;
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

sevdb_database* sevdb_db_create(uint32_t capacity){
    sevdb_database *db = malloc(sizeof(sevdb_database));
    if (db == NULL) return NULL;
    db->capacity = capacity;
    db->count = 0;
    db->vectors = calloc(capacity, sizeof(sevdb_vector*));
    if (db->vectors == NULL){
        free(db);
        return NULL;
    }
    return db;
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
    free(db);
}


bool sevdb_db_increase_capacity(sevdb_database *db, uint32_t increase){
    if (increase == 0) return false;
    uint64_t new_capacity = db->capacity + increase;
    if (new_capacity > UINT32_MAX) return false;

    size_t new_size = new_capacity * sizeof(sevdb_vector*);
    sevdb_vector** temp = realloc(db->vectors, new_size);
    if (temp == NULL) return false;
    memset(temp + db->capacity, 0,(new_capacity - db->capacity) * sizeof(sevdb_vector*));

    db->capacity = (uint32_t)new_capacity;
    db->vectors = temp; 
    return true;
}

uint32_t sevdb_db_get_capacity(const sevdb_database *db){
    assert(db != NULL);
    return db->capacity;
}

uint32_t sevdb_db_get_count(const sevdb_database *db){
    assert(db != NULL);
    return db->count;
}

sevdb_vector* sevdb_db_push_vector(sevdb_database *db, sevdb_vector *v){
    if (db == NULL || v == NULL) return NULL;
    if (db->count >= db->capacity) return NULL;
    //serch for free spot
    uint32_t free_slot; 
    bool found_free_slot = false;
    for(uint32_t i = 0; i < db->capacity; i++){
        if (db->vectors[i] != NULL){
            if (db->vectors[i]->id == v->id) return NULL;
            continue;
        }
        if (!found_free_slot){
            free_slot = i;
            found_free_slot = true;
        }
    }
    if (found_free_slot){
        db->vectors[free_slot] = v;
        db->count++;
        return v;
    }
    return NULL;
}

sevdb_vector* sevdb_db_get_vector_by_id(sevdb_database *db, uint32_t id){
    if (db == NULL || db->count == 0) return NULL;
    sevdb_vector * v = NULL;
    for(uint32_t i = 0; i < db->capacity; i++){
        v = db->vectors[i];
        if (v != NULL && v->id == id){
            return v; 
        }
    }
    return NULL;
}

void sevdb_db_remove_vector_by_id(sevdb_database *db, uint32_t id){
    if (db == NULL || db->count == 0) return;
    sevdb_vector * v = NULL;
    for(uint32_t i = 0; i < db->capacity; i++){
        v = db->vectors[i];
        if (v != NULL && v->id == id){
            sevdb_vector_destroy(v);
            db->vectors[i]=NULL;
            db->count--;
            return;
        }
    }
}

static bool is_max_heap(float a, float b) { return a > b; }
static bool is_min_heap(float a, float b) { return a < b; }

int sevdb_db_search_k_similar_vectors(sevdb_database *db, const sevdb_vector* v, int k, sevdb_similarity_scored_vector* out_results){
    if (db == NULL || db->count == 0 || v == NULL || out_results == NULL) return 0;

    pqueue* pq = pq_create(k, is_min_heap);
    if (pq == NULL) return 0;

    sevdb_vector* candidate;
    pq_element worst_saved_el;
    float candidate_cos_sim;

    for(uint32_t i = 0; i < db->capacity; i++){
        candidate = db->vectors[i];
        if (candidate == NULL || candidate->dimensions != v->dimensions) {
            continue; 
        }

        candidate_cos_sim = get_cosine_similarity(v->dimensions, v->components, candidate->components);

        if (pq_get_count(pq) < pq_get_capacity(pq)){
            if (!pq_enqueue(pq, candidate_cos_sim, candidate)) {
                pq_destroy(pq);
                return 0;
            }
        } else {
            if (!pq_peek(pq, &worst_saved_el)){
                pq_destroy(pq);
                return 0;
            }

            if (candidate_cos_sim > worst_saved_el.priority){
                if (!(pq_dequeue(pq, &worst_saved_el) && pq_enqueue(pq, candidate_cos_sim, candidate))){
                    pq_destroy(pq);
                    return 0;
                }
            }
        }
    }

    int retrieved = pq_get_count(pq);
    pq_element out;

    for(int i = retrieved - 1; i >= 0; i--){
        if  (pq_dequeue(pq, &out)) {
            out_results[i].vector = (sevdb_vector*)out.content;
            out_results[i].score = out.priority;
        }
    }

    pq_destroy(pq);
    return retrieved; 
}
