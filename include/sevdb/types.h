#pragma once

/**
 * @struct sevdb_vector
 * @brief The fundamental building block of the database.
 * 
 * @note Represents a single vector with a unique identifier and a dynamic array of 
 * floating-point components.
 */
typedef struct sevdb_vector sevdb_vector;

/**
 * @struct sevdb_database
 * @brief The core database engine instance.
 * 
 * @note Stores all vectors and manages their memory lifecycle. Enables adding, 
 * deleting, and performing Top-K similarity searches across the dataset.
 */
typedef struct sevdb_database sevdb_database;


/**
 * @struct sevdb_similarity_scored_vector
 * @brief A vector with its similarity score
 *
 * @note Couples a vector with it's simillarity score, used as return type for a similarity search.
 */

typedef struct sevdb_similarity_scored_vector{
    sevdb_vector* vector;
    float score;
}sevdb_similarity_scored_vector;
