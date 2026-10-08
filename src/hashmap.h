#pragma once
#include <stdint.h>
#include <stdbool.h>

/**
 * @struct hm_element
 * @brief A single key-value pair stored in the hash map.
 * 
 * @var hm_element::key The unique identifier for the element.
 * @var hm_element::value The data associated with the key.
 * @var hm_element::used The current state of the slot (EMPTY, USED, or DELETED).
 */
typedef struct hm_element {
    uint32_t key;
    uint32_t value;
    char used;
} hm_element;

/**
 * @struct hashmap
 * @brief A linear-probing hash map for storing key-value pairs in O(1) average time.
 * 
 * @var hashmap::capacity The total number of allocated slots in the hash map.
 * @var hashmap::count The number of active elements currently stored.
 * @var hashmap::elements Pointer to the contiguous array of hm_element slots.
 */
typedef struct hashmap {
    uint32_t capacity;
    uint32_t count;
    hm_element *elements;
} hashmap;

/**
 * @brief Allocates and initializes a new hash map.
 * 
 * @param capacity The initial number of slots to allocate (must be > 0).
 * @return A pointer to the newly allocated hash map, or NULL if allocation fails.
 * @note The returned hash map is allocated on the heap and must be freed using hm_destroy.
 */
hashmap* hm_create(uint32_t capacity);

/**
 * @brief Safely destroys the hash map and frees all associated memory.
 * 
 * @param hm Pointer to the hash map to destroy. Safely handles NULL.
 * @return true on success, false if hm was NULL.
 */
bool hm_destroy(hashmap *hm);

/**
 * @brief Inserts a new key-value pair into the hash map.
 * 
 * Automatically handles growing the hash map if the load factor exceeds 70%.
 * 
 * @param hm Pointer to the hash map.
 * @param key The unique key to insert.
 * @param value The value to associate with the key.
 * @return true on successful insertion, false if the key already exists or memory allocation fails.
 */
bool hm_insert_new(hashmap* hm, uint32_t key, uint32_t value);

/**
 * @brief Updates the value of an existing key in the hash map.
 * 
 * @param hm Pointer to the hash map.
 * @param key The key whose value should be updated.
 * @param value The new value to assign.
 * @return true if the update was successful, false if the key was not found.
 */
bool hm_update_value(hashmap* hm, uint32_t key, uint32_t value);

/**
 * @brief Checks if a specific key is present in the hash map.
 * 
 * @param hm Pointer to the hash map.
 * @param key The key to search for.
 * @return true if the key exists, false otherwise.
 */
bool hm_contains_key(const hashmap* hm, uint32_t key);

/**
 * @brief Retrieves the value associated with a specific key.
 * 
 * @param hm Pointer to the hash map.
 * @param key The key to search for.
 * @param out_value Pointer to a variable where the found value will be written.
 * @return true if the key was found and out_value populated, false if the key is missing.
 */
bool hm_get_value(const hashmap* hm, uint32_t key, uint32_t *out_value);

/**
 * @brief Removes an element from the hash map.
 * 
 * Uses a tombstone (DELETED) marker to maintain linear probing chains for other elements.
 * 
 * @param hm Pointer to the hash map.
 * @param key The key of the element to remove.
 * @return true if the element was successfully found and removed, false if the key is missing.
 */
bool hm_remove(hashmap* hm, uint32_t key);
