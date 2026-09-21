#pragma once
#include <stdbool.h>

/**
 * @struct sevdb_vector
 * @brief The fundamental building block of the database.
 * 
 * Represents a single vector with a unique identifier and a dynamic array of 
 * floating-point components.
 */
typedef struct sevdb_vector sevdb_vector;

/**
 * @struct sevdb_database
 * @brief The core database engine instance.
 * 
 * Stores all vectors and manages their memory lifecycle. Enables adding, 
 * deleting, and performing Top-K similarity searches across the dataset.
 */
typedef struct sevdb_database sevdb_database;

// ==========================================
// == VECTOR LIFECYCLE & OPERATIONS        ==
// ==========================================
  
/**
 * @brief Allocates and initializes a new vector on the heap.
 * 
 * @param id The unique identifier for this vector.
 * @param dimensions The number of components in the vector. Must be > 0.
 * @param components Pointer to a float array containing the vector data. 
 *                   The data is copied internally, so the caller can safely free their own array after this call.
 * @return A pointer to the newly allocated vector, or NULL if allocation fails or invalid parameters are provided.
 */
sevdb_vector* sevdb_vector_create(int id, int dimensions, const float *components);

/**
 * @brief Safely frees the memory associated with a vector.
 * 
 * @param v Pointer to the vector to destroy. Safely handles NULL.
 */
void sevdb_vector_destroy(sevdb_vector *v);

// --- Vector Getters ---
int sevdb_vector_get_id(sevdb_vector *v);
int sevdb_vector_get_dimensions(sevdb_vector *v);
float* sevdb_vector_get_components(sevdb_vector *v);


// ==========================================
// == DATABASE LIFECYCLE & OPERATIONS      ==
// ==========================================
  
/**
 * @brief Allocates and initializes a new vector database.
 * 
 * @param capacity The maximum number of vectors this database can store.
 *
 * @return A pointer to the new database, or NULL if allocation fails.
 */
sevdb_database* sevdb_db_create(int capacity);

/**
 * @brief Destroys the database and frees ALL vectors stored within it.
 * 
 * @param db Pointer to the database. Safely handles NULL.
 */
void sevdb_db_destroy(sevdb_database *db);
 
// --- Database Getters ---
int sevdb_database_get_capacity(sevdb_database *db);
int sevdb_database_get_count(sevdb_database *db);

/**
 * @brief Inserts a vector into the database.
 * 
 * @param db Pointer to the database instance.
 * @param v Pointer to the vector to insert. 
 *
 * @note **Ownership Transfer:** If successful, the database assumes ownership of the vector's memory. 
 *      Do not manually call sevdb_vector_destroy on it.
 * @return The inserted vector on success, or NULL if the database is full, arguments are NULL, or insertion fails.
 */
sevdb_vector* sevdb_db_push_vector(sevdb_database *db, sevdb_vector *v);

/**
 * @brief Retrieves a vector from the database by its ID.
 * 
 * @return A pointer to the vector, or NULL if the ID is not found. 
 *         The database retains ownership of the returned memory.
 */
sevdb_vector* sevdb_db_get_vector_by_id(sevdb_database *db, int id);

/**
 * @brief Removes a vector from the database by its ID and frees its memory.
 * 
 * @param db Pointer to the database instance.
 * @param id The identifier of the vector to remove.
 */
void sevdb_db_remove_vector_by_id(sevdb_database *db, int id);

/**
 * @brief Performs a Top-K similarity search using Cosine Similarity.
 * 
 * @param db Pointer to the database to search.
 * @param v The target vector to compare against.
 * @param k The maximum number of similar vectors to return.
 * @param out_vector_list A caller-allocated array of vector pointers (size >= k) that will be populated with the best matches.
 * @return The actual number of vectors found and placed into out_vector_list.
 */
int sevdb_db_search_k_similar_vectors(sevdb_database *db, sevdb_vector* v, int k, sevdb_vector** out_vector_list);


// ==========================================
// == DATA PERSISTENCE                     ==
// ==========================================

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

/**
 * @brief Serializes the entire database state to a binary file on disk.
 * 
 * @param db The database to serialize.
 * @param path The filepath where the binary file will be created/overwritten.
 * @return True on success, False if file operations fail.
 */
bool sevdb_db_serialize(sevdb_database *db, const char *path);

/**
 * @brief Deserializes a binary database file from disk into memory.
 * 
 * @param path The filepath of the binary file to read.
 * @return A pointer to the reconstructed database, or NULL on failure.
 */
sevdb_database* sevdb_db_deserialize(const char *path);
