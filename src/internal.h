#pragma once
// This struct rappresents the foundamental block of the database.
// The sevdb_vector is the item that will be saved and queried in this db.
typedef struct {
    int id;                 //unique identifier 
    int dimensions;         //number of dimensions (components) of the vector
    float components[];     //array of components
} sevdb_vector;

// This struct rappresents the actual db
// The sevdb_database is the structure that will store all the vectors,
// enabling adding new vectors, deleting existing vectors, 
// and searching for specific and similar vectors.
typedef struct {
    int capacity;           //how many vector can the db store
    int count;              //how many vector are currently stored
    sevdb_vector **vectors; //array of vectors (pointer to the first)
} sevdb_database;

// Vector constructor
sevdb_vector* sevdb_vector_create(int id, int dimensions, const float *components);
// Vector destructor 
void sevdb_vector_destroy(sevdb_vector *v);

// Database constructor
sevdb_database* sevdb_db_create(int capacity);
// Database destructor
void sevdb_db_destroy(sevdb_database *db);
