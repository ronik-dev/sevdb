#include "internal.h"
#include "distance.h"
#include "pqueue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#define MAGIC "SEVDB"
#define MAGIC_SIZE 5
#define SEVDB_MAX_DIMENSIONS 4096
const uint32_t version = 1;

struct sevdb_vector{
    uint32_t id;            //unique identifier 
    uint32_t dimensions;    //number of dimensions (components) of the vector
    float components[];     //array of components
};

struct sevdb_database{
    uint32_t capacity;      //how many vector can the db store
    uint32_t count;         //how many vector are currently stored
    sevdb_vector **vectors; //array of vectors (pointer to the first)
};


//----------
//--VECTOR--
//----------

sevdb_vector* sevdb_vector_create(uint32_t id, uint32_t dimensions ,const float* components){
    if(dimensions < 0 || components == NULL) return NULL;
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

uint32_t sevdb_vector_get_id(sevdb_vector *v){
    return v->id;
}

uint32_t sevdb_vector_get_dimensions(sevdb_vector *v){
    return v->dimensions;
}

float* sevdb_vector_get_components(sevdb_vector *v){
    return v->components;
}



//------------
//--DATABASE--
//------------

sevdb_database* sevdb_db_create(uint32_t capacity){
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

uint32_t sevdb_database_get_capacity(sevdb_database *db){
    return db->capacity;
}

uint32_t sevdb_database_get_count(sevdb_database *db){
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

void sevdb_db_remove_vector_by_id(sevdb_database *db, uint32_t id){
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

//---------------
//--PERSISTENCE--
//---------------

const bool sevdb_vector_serialize(FILE* fp, sevdb_vector* v){
    if (v == NULL || fp == NULL) return false;
    // serialize id
    if (fwrite(&v->id, sizeof(v->id), 1, fp) != 1){
        return false;
    }

    // serialize dimensions
    if (fwrite(&v->dimensions, sizeof(v->dimensions), 1, fp) != 1){
        return false;
    }

    // serialize components 
    if (fwrite(v->components, sizeof(*v->components), v->dimensions, fp) != v->dimensions){
        return false;
    }

    return true;
}


bool sevdb_db_serialize(sevdb_database *db, const char *path) {
    if (db == NULL || path == NULL) return false;

    uint32_t serialized_count = 0;
    for (uint32_t i = 0; i < db->capacity; i++) {
        if (db->vectors[i] != NULL) serialized_count++;
    }
    if (serialized_count != db->count) return false;

    char temp_path[PATH_MAX];
    int n = snprintf(temp_path, sizeof(temp_path), "%s.XXXXXX", path);
    if (n < 0 || (size_t)n >= sizeof(temp_path)) return false;

    int fd = mkstemp(temp_path);
    if (fd == -1) return false;

    FILE* fp_temp = fdopen(fd, "wb");
    if (fp_temp == NULL) {
        close(fd);
        remove(temp_path);
        return false;
    }

    if (fwrite(MAGIC, MAGIC_SIZE, 1, fp_temp) != 1) goto fail;
    if (fwrite(&version, sizeof(version), 1, fp_temp) != 1) goto fail;
    if (fwrite(&db->capacity, sizeof(db->capacity), 1, fp_temp) != 1) goto fail;
    if (fwrite(&db->count, sizeof(db->count), 1, fp_temp) != 1) goto fail;

    for (uint32_t i = 0; i < db->capacity; i++) {
        if (db->vectors[i] == NULL) continue;
        if (!sevdb_vector_serialize(fp_temp, db->vectors[i])) goto fail;
    }

    if (fclose(fp_temp) != 0) {
        remove(temp_path);
        return false;
    }

    if (rename(temp_path, path) != 0) {
        remove(temp_path);
        return false;
    }

    return true;

fail:
    fclose(fp_temp);
    remove(temp_path);
    return false;
}

/*
 * SEVDB file format v1
 *
 * Header:
 *   char[5]   magic = "SEVDB"
 *   uint32_t  version = 1
 *   uint32_t  capacity
 *   uint32_t  count
 *
 * Vector:
 *   uint32_t  id
 *   uint32_t  dimensions
 *   float     components[dimensions]
 */
sevdb_database* sevdb_db_deserialize_v1(FILE* fp) {
    if (fp == NULL) return NULL;

    uint32_t capacity;
    if (fread(&capacity, sizeof(capacity), 1, fp) != 1)
        return NULL;

    uint32_t count;
    if (fread(&count, sizeof(count), 1, fp) != 1)
        return NULL;
    if (count > capacity) return NULL;

    sevdb_database *new_db = sevdb_db_create(capacity);
    if (new_db == NULL) return NULL;

    for (uint32_t v = 0; v < count; v++) {
        uint32_t v_id;
        if (fread(&v_id, sizeof(v_id), 1, fp) != 1) {
            sevdb_db_destroy(new_db);
            return NULL;
        }

        uint32_t v_dimensions;
        if (fread(&v_dimensions, sizeof(v_dimensions), 1, fp) != 1) {
            sevdb_db_destroy(new_db);
            return NULL;
        }
        if (v_dimensions == 0 || v_dimensions > SEVDB_MAX_DIMENSIONS) {
            sevdb_db_destroy(new_db);
            return NULL;
        }

        float *v_components = malloc(sizeof(float) * v_dimensions);
        if (v_components == NULL) {
            sevdb_db_destroy(new_db);
            return NULL;
        }

        if (fread(v_components, sizeof(float), v_dimensions, fp) != v_dimensions) {
            free(v_components);
            sevdb_db_destroy(new_db);
            return NULL;
        }

        sevdb_vector *new_vector = sevdb_vector_create(v_id, v_dimensions, v_components);
        free(v_components); // sevdb_vector_create memcpy's the components, safe to free now
        if (new_vector == NULL) {
            sevdb_db_destroy(new_db);
            return NULL;
        }
        if (sevdb_db_push_vector(new_db, new_vector) == NULL) {
            sevdb_vector_destroy(new_vector);
            sevdb_db_destroy(new_db);
            return NULL;
        }
    }

    return new_db;
}

sevdb_database* sevdb_db_deserialize(const char *path) {
    FILE* fp = fopen(path, "rb");

    if (fp == NULL) {
        return NULL;
    }

    sevdb_database* new_db = NULL;

    char magic_buffer[MAGIC_SIZE];

    if (fread(magic_buffer, MAGIC_SIZE, 1, fp) != 1) {
        fclose(fp);
        return NULL;
    }

    if (memcmp(MAGIC, magic_buffer, MAGIC_SIZE) != 0) {
        fclose(fp);
        return NULL;
    }

    uint32_t file_version;

    if (fread(&file_version, sizeof(file_version), 1, fp) != 1) {
        fclose(fp);
        return NULL;
    }

    switch (file_version) {
        case 1:
            new_db = sevdb_db_deserialize_v1(fp);
            break;

        default:
            fclose(fp);
            return NULL;
    }

    fclose(fp);

    return new_db;
}
