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
 */
typedef struct hashmap hashmap;

typedef enum hm_state {
    HM_OK = 0,                      //Operation succeded :)

    HM_ERROR_INVALID_ARGUMENT,      //Function called with unacceptable :( argument, try again.
    HM_ERROR_FULL,                  //Hash map has reached it maximum capacity
    HM_ERROR_DUPLICATE_KEY,         //Tryed to make an operation that would result in duplicate keys (not permitted)
    HM_ERROR_NOT_FOUND,             //Tryed to make an operation on an unexisting key
    HM_ERROR_OUT_OF_MEMORY,         //We have run out of memory :¬|
    HM_ERROR_INCONSISTENT_STATE     //CRITICAL! We reached an uncosistent state
} hm_state;

/**
 * @brief Allocates and initializes a new hash map.
 * 
 * @param capacity The initial number of slots to allocate (must be > 0).
 * @param out_result Pointer to a variable where the newly allocated hash map will be written.
 * @return hm_state HM_OK if the operation succeeded, or a specific error code (e.g., HM_ERROR_OUT_OF_MEMORY).
 * @note The returned hash map is allocated on the heap and must be freed using hm_destroy.
 */
hm_state hm_create(uint32_t capacity, hashmap** out_result);

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
 * @return hm_state HM_OK if the operation succeeded, HM_ERROR_DUPLICATE_KEY if it already exists, or other error codes.
 */
hm_state hm_insert_new(hashmap* hm, uint32_t key, uint32_t value);

/**
 * @brief Updates the value of an existing key in the hash map.
 * 
 * @param hm Pointer to the hash map.
 * @param key The key whose value should be updated.
 * @param value The new value to assign.
 * @return hm_state HM_OK if the operation succeeded, or HM_ERROR_NOT_FOUND if the key does not exist.
 */
hm_state hm_update_value(hashmap* hm, uint32_t key, uint32_t value);

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
 * @return hm_state HM_OK if the operation succeeded, or HM_ERROR_NOT_FOUND.
 */
hm_state hm_get_value(const hashmap* hm, uint32_t key, uint32_t *out_value);

/**
 * @brief Removes an element from the hash map.
 * 
 * Uses a tombstone (DELETED) marker to maintain linear probing chains for other elements.
 * 
 * @param hm Pointer to the hash map.
 * @param key The key of the element to remove.
 * @return hm_state HM_OK if the operation succeeded, or HM_ERROR_NOT_FOUND.
 */
hm_state hm_remove(hashmap* hm, uint32_t key);

/**
 * @brief Get hashmap capacity
 * 
 * @param hm Pointer to the hash map.
 * @param out_result Pointer to a variable where the retrieved capacity will be written.
 * @return true if the operation was successful, and the result got saved in out_result.
 *         false if the operation was not successful (e.g., NULL pointer).
 */
bool hm_get_capacity(const hashmap* hm, uint32_t* out_result);

/**
 * @brief Get hashmap current count
 * 
 * @param hm Pointer to the hash map.
 * @param out_result Pointer to a variable where the retrieved count will be written.
 * @return true if the operation was successful, and the result got saved in out_result.
 *         false if the operation was not successful (e.g., NULL pointer).
 */
bool hm_get_count(const hashmap* hm, uint32_t* out_result);
