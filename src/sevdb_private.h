#pragma once
#include <stdint.h>

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
