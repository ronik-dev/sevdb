#pragma once
// This struct rappresents the foundamental block of the database.
// The sevdb_vector is the item that will be saved and queried in this db.
typedef struct sevdb_vector sevdb_vector;

// This struct rappresents the actual db
// The sevdb_database is the structure that will store all the vectors,
// enabling adding new vectors, deleting existing vectors, 
// and searching for specific and similar vectors.
typedef struct sevdb_database sevdb_database;

//----------
//--VECTOR--
//----------
  
// sevdb_vector constructor
sevdb_vector* sevdb_vector_create(int id, int dimensions, const float *components);
// sevdb_vector destructor
void sevdb_vector_destroy(sevdb_vector *v);

// getters

int sevdb_vector_get_id(sevdb_vector *v);
int sevdb_vector_get_dimensions(sevdb_vector *v);
float* sevdb_vector_get_components(sevdb_vector *v);


//------------
//--DATABASE--
//------------
  
// sevdb_database constructor
sevdb_database* sevdb_db_create(int capacity);
// sevdb_database destructor
void sevdb_db_destroy(sevdb_database *db);
 
// getters
  
int sevdb_database_get_capacity(sevdb_database *db);
int sevdb_database_get_count(sevdb_database *db);

// database operations
sevdb_vector* sevdb_db_push_vector(sevdb_database *db, sevdb_vector *v);
sevdb_vector* sevdb_db_get_vector_by_id(sevdb_database *db, int id);
void sevdb_db_remove_vector_by_id(sevdb_database *db, int id);
