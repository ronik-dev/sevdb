#include "sevdb/sevdb.h"
#include "sevdb_private.h"
#include "checksum.h"            
#include <string.h>    
#include <unistd.h>
#include <limits.h> 
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define MAGIC "SEVDB"
#define MAGIC_SIZE 5
const uint32_t version = 1;

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

    FILE* fp_temp = fdopen(fd, "wb+");
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

    if (fflush(fp_temp) != 0) goto fail;
    rewind(fp_temp); 

    uint32_t calculated_crc = 0;
    uint8_t chunk[4096];
    size_t bytes_read;
    
    while ((bytes_read = fread(chunk, 1, sizeof(chunk), fp_temp)) > 0) {
        calculated_crc = crc32(calculated_crc, chunk, bytes_read);
    }
    
    if (ferror(fp_temp)) goto fail;

    clearerr(fp_temp);

    if (fwrite(&calculated_crc, sizeof(calculated_crc), 1, fp_temp) != 1) goto fail;

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
        if (v_dimensions == 0 || v_dimensions > SEVDB_MAX_VECTOR_DIMENSIONS) {
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
    
    long current_pos = ftell(fp);
    
    if (fseek(fp, -((long)sizeof(uint32_t)), SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    
    uint32_t saved_crc;
    if (fread(&saved_crc, sizeof(saved_crc), 1, fp) != 1) {
        fclose(fp);
        return NULL;
    }
    
    long data_length = ftell(fp) - sizeof(uint32_t);
    
    rewind(fp);
    uint32_t calculated_crc = 0;
    uint8_t chunk[4096];
    long bytes_remaining = data_length;
    
    while (bytes_remaining > 0) {
        size_t to_read = (bytes_remaining < sizeof(chunk)) ? bytes_remaining : sizeof(chunk);
        size_t bytes_read = fread(chunk, 1, to_read, fp);
        
        if (bytes_read != to_read) {
            fclose(fp);
            return NULL;
        }
        
        calculated_crc = crc32(calculated_crc, chunk, bytes_read);
        bytes_remaining -= bytes_read;
    }
    
    if (calculated_crc != saved_crc) {
        fclose(fp);
        return NULL; // CORRUPTED FILE
    }
    
    // . Restore the file pointer to where it was so deserialization can continue normally
    fseek(fp, current_pos, SEEK_SET);

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
