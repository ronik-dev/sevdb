#pragma once

#ifdef __cplusplus
extern "C" {
#endif

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

/**
 * @enum sevdb_state
 * @brief all failure conditions
 */
typedef enum{
    //OK
    SEVDB_OK = 0,

    //FUNCTION ARGUMENTS
    SEVDB_ERROR_INVALID_ARGUMENT,

    //DATABASE INTERNAL STATUS
    SEVDB_ERROR_DATABASE_FULL,      //The database reached its maximum capacity
    SEVDB_ERROR_DUPLICATE_ID,       //Inconsistent state: there are two entries with the same ID
    SEVDB_ERROR_NOT_FOUND,          //Requested an operation on a non existing element
    SEVDB_ERROR_INTERNAL,           //Inconsistent state: internal error
    SEVDB_ERROR_OUT_OF_MEMORY,  
    SEVDB_ERROR_OVERFLOW
}sevdb_state;

#ifdef __cplusplus
}
#endif
