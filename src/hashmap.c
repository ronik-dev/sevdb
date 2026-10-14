#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

#define EMPTY   0
#define USED    1
#define DELETED 2



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

hm_state hm_create(uint32_t capacity, hashmap** out_result){
    if (capacity == 0 || out_result == NULL) return HM_ERROR_INVALID_ARGUMENT;
    hashmap* hm = malloc(sizeof(hashmap));
    if (hm == NULL) return HM_ERROR_OUT_OF_MEMORY;
    hm->capacity= capacity;
    hm->count = 0;
    hm->elements = calloc(capacity, sizeof(hm_element));
    if (hm->elements == NULL){
        free(hm);
        return HM_ERROR_OUT_OF_MEMORY;
    }
    *out_result = hm;
    return HM_OK;
}

bool hm_destroy(hashmap *hm){
    if (hm == NULL) return false;
    if (hm->elements != NULL) free(hm->elements);
    free(hm);
    return true;
}

/**
 * @brief Computes the array index for a given integer key.
 * 
 * Since our keys are already unique integers (e.g., vector IDs), we don't 
 * need a complex hashing algorithm to scramble the bits. The modulo 
 * operator (%) simply wraps the integer to safely fit within the array 
 * bounds (0 to capacity - 1), ensuring valid memory access.
 */
uint32_t hm_hash(uint32_t key, uint32_t capacity){
    return key % capacity;
}

/**
 * @brief Expands the hash map and re-distributes all existing elements.
 * 
 * We cannot simply use `realloc` to resize the underlying array. Because 
 * our hash function dictates that a bucket's index is (key % capacity), 
 * changing the capacity changes the correct bucket for almost every existing key. 
 * If we didn't move them, future searches would look in the wrong buckets.
 * Therefore, we must allocate a fresh array and re-hash every USED element 
 * into its new, mathematically correct position.
 */
bool hm_grow(hashmap *hm){
    if (hm == NULL) return false;
    uint32_t new_capacity;
    if (hm->capacity > UINT32_MAX / 2){
        new_capacity = UINT32_MAX;
    } else {
        new_capacity = hm->capacity * 2;
    }

    hm_element* new_elements = calloc(new_capacity, sizeof(hm_element));
    if (new_elements == NULL) return false;
    
    for (uint32_t i = 0; i < hm->capacity; i++) {
        if (hm->elements[i].used == USED) {
            uint32_t key = hm->elements[i].key;
            uint32_t new_i = hm_hash(key, new_capacity);
            
            while (new_elements[new_i].used == USED) {
                new_i = hm_hash(new_i + 1, new_capacity);
            }
            
            new_elements[new_i].key = key;
            new_elements[new_i].value = hm->elements[i].value;
            new_elements[new_i].used = USED;
        }
    }
    
    free(hm->elements);
    hm->capacity = new_capacity;
    hm->elements = new_elements;
    return true;
}


hm_element* search (const hashmap* hm, uint32_t key){
    if (hm == NULL) return NULL;
    uint32_t i = hm_hash(key, hm->capacity);
    while(hm->elements[i].used != EMPTY){
        if (hm->elements[i].used == USED && hm->elements[i].key == key){
            return &hm->elements[i];
        }
        i = hm_hash(i+1,hm->capacity);
    }
    return NULL;
}


/**
 * insert a new key value pair in the hashmap.
 * To keep the search time close to O(1), we grow the hashmap when
 * count/capacity ratio gets over 70%.
 * A full hash map has a search time of O(N).
 * We still cant grow over the max capacity of the hashmap
 */
hm_state hm_insert_new(hashmap* hm, uint32_t key, uint32_t value){
    if (hm == NULL) return HM_ERROR_INVALID_ARGUMENT;
    
    // duplicate detection
    uint32_t i = hm_hash(key, hm->capacity);
    if(search(hm, key) != NULL){
        return HM_ERROR_DUPLICATE_KEY;    
    }
    
    // grow
    if (hm->count >= hm->capacity * 0.7f || hm->count >= hm->capacity - 1){
        if (hm->count == UINT32_MAX) return HM_ERROR_FULL;
        if (!hm_grow(hm)) return HM_ERROR_OUT_OF_MEMORY;
        i = hm_hash(key, hm->capacity); 
    }
    
    if (hm->count >= hm->capacity-1) return HM_ERROR_FULL;
    
    while(hm->elements[i].used == USED){
        i = hm_hash(i+1, hm->capacity);
    }
    hm->elements[i].used = USED;
    hm->elements[i].key = key;
    hm->elements[i].value = value;
    hm->count++;
    
    return HM_OK;
}

hm_state hm_update_value(hashmap* hm, uint32_t key, uint32_t value){
    if (hm == NULL) return HM_ERROR_INVALID_ARGUMENT;
    hm_element* duplicate = search(hm, key);
    if(duplicate){
        duplicate->value = value;
        return HM_OK;    
    }
    return HM_ERROR_NOT_FOUND;
}

bool hm_contains_key(const hashmap* hm, uint32_t key){
    if (hm == NULL) return false;
    hm_element* target = search(hm, key);
    if(target == NULL) return false;
    return true;
}

hm_state hm_get_value(const hashmap* hm, uint32_t key, uint32_t *out_value){
    if (hm == NULL || out_value == NULL) return HM_ERROR_INVALID_ARGUMENT;
    hm_element* target = search(hm, key);
    if(target == NULL) return HM_ERROR_NOT_FOUND;
    *out_value = target->value;
    return HM_OK;
}

/**
 * removes an element from the hash map by putting the value to 0
 * and the used status to DELETED.
 * It is important to use DELETED and not EMPTY.
 * Using EMPTY would make all the following elements in the hashmap
 * unreachable during search.
 */
hm_state hm_remove(hashmap* hm, uint32_t key){
    if (hm == NULL) return HM_ERROR_INVALID_ARGUMENT;
    hm_element* target = search(hm,key);
    if (target == NULL) return HM_ERROR_NOT_FOUND;
    target->value = 0;
    target->used= DELETED;
    hm->count--;
    return HM_OK;
}

bool hm_get_capacity(const hashmap* hm, uint32_t* out_result){
    if (hm == NULL || out_result == NULL) return false;
    *out_result = hm->capacity;
    return true;
}

bool hm_get_count(const hashmap* hm, uint32_t* out_result){
    if (hm == NULL || out_result == NULL) return false;
    *out_result = hm->count;
    return true;
}
