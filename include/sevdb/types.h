#pragma once

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
