#include "internal.h"
#include "distance.h"
#include "pqueue.h"
#include <stdlib.h>
#include <string.h>

struct sevdb_vector{
    int id;                 //unique identifier 
    int dimensions;         //number of dimensions (components) of the vector
    float components[];     //array of components
};

struct sevdb_database{
    int capacity;           //how many vector can the db store
    int count;              //how many vector are currently stored
    sevdb_vector **vectors; //array of vectors (pointer to the first)
};


//----------
//--VECTOR--
//----------
  
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

int sevdb_vector_get_id(sevdb_vector *v){
    return v->id;
}

int sevdb_vector_get_dimensions(sevdb_vector *v){
    return v->dimensions;
}

float* sevdb_vector_get_components(sevdb_vector *v){
    return v->components;
}



//------------
//--DATABASE--
//------------

sevdb_database* sevdb_db_create(int capacity){
    sevdb_database *db = malloc(sizeof(sevdb_database));
    if(db == NULL) return NULL;
    db->capacity = capacity;
    db->count = 0;
    db->vectors = calloc(capacity, sizeof(sevdb_vector*));
    if(db->vectors == NULL){
        free(db);
        return NULL;
    }
    return db;
}

void sevdb_db_destroy(sevdb_database *db){
    if(db == NULL) return; 
    if(db->vectors != NULL){
        for (int i = 0; i < db->capacity; i++){
            if(db->vectors[i] != NULL){
                sevdb_vector_destroy(db->vectors[i]);  
            }
        }
        free(db->vectors);
    }
    free(db);
}

int sevdb_database_get_capacity(sevdb_database *db){
    return db->capacity;
}

int sevdb_database_get_count(sevdb_database *db){
    return db->count;
}
  
sevdb_vector* sevdb_db_push_vector(sevdb_database *db, sevdb_vector *v){
    if(db == NULL || v == NULL) return NULL;
    if(db->count >= db->capacity) return NULL;
    //serch for free spot
    for(int i = 0; i < db->capacity; i++){
        if(db->vectors[i] == NULL){
            db->vectors[i] = v;
            db->count++;
            return v;
        }
    }
    return NULL;
}

sevdb_vector* sevdb_db_get_vector_by_id(sevdb_database *db, int id){
    if(db == NULL || db->count == 0) return NULL;
    sevdb_vector * v = NULL;
    for(int i = 0; i < db->capacity; i++){
        v = db->vectors[i];
        if(v != NULL && v->id == id){
            return v; 
        }
    }
    return NULL;
}

void sevdb_db_remove_vector_by_id(sevdb_database *db, int id){
    if(db == NULL || db->count == 0) return;
    sevdb_vector * v = NULL;
    for(int i = 0; i < db->capacity; i++){
        v = db->vectors[i];
        if(v != NULL && v->id == id){
            sevdb_vector_destroy(v);
            db->vectors[i]=NULL;
            db->count--;
            return;
        }
    }
}

static bool is_max_heap(float a, float b) { return a > b; }
static bool is_min_heap(float a, float b) { return a < b; }

int sevdb_db_search_k_similar_vectors(sevdb_database *db, sevdb_vector* v, int k, sevdb_vector** out_vector_list){
    if(db == NULL || db->count == 0 || v == NULL || out_vector_list == NULL) return 0;
    
    pqueue* pq = pq_create(k, is_min_heap);
    if(pq == NULL) return 0;

    sevdb_vector* candidate;
    pq_element worst_saved_el;
    float candidate_cos_sim;
    
    for(int i = 0; i < db->capacity; i++){
        candidate = db->vectors[i];
        if(candidate == NULL || candidate->dimensions != v->dimensions) {
            continue; 
        }
        
        candidate_cos_sim = get_cosine_similarity(v->dimensions, v->components, candidate->components);
        
        if(pq_get_count(pq) < pq_get_capacity(pq)){
            if (!pq_enqueue(pq, candidate_cos_sim, candidate)) {
                pq_destroy(pq);
                return 0;
            }
        } else {
            if(!pq_peek(pq, &worst_saved_el)){
                pq_destroy(pq);
                return 0;
            }
            
            if(candidate_cos_sim > worst_saved_el.priority){
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
        if(pq_dequeue(pq, &out)) {
            out_vector_list[i] = (sevdb_vector*)out.content;
        }
    }
    
    pq_destroy(pq);
    return retrieved; 
}
