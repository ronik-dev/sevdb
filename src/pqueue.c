#include "pqueue.h"
#include <stdlib.h>
#include <math.h>

struct pqueue {
    int capacity; 
    int count;
    bool(*comparator)(float,float);
    pq_element *elements;
};

static void swap(pq_element* pq_el1, pq_element* pq_el2){
    pq_element temp = *pq_el1;
    *pq_el1 = *pq_el2;
    *pq_el2 = temp;
}

static void heapify_up(pqueue* pq, int index){
    if(index > 0
       //&& pq->elements[(index - 1) / 2].priority > pq->elements[index].priority) {
       && pq->comparator( pq->elements[index].priority, pq->elements[(index - 1) / 2].priority)) {
        swap(&pq->elements[(index - 1) / 2],
             &pq->elements[index]);
        heapify_up(pq, (index - 1) / 2);
    }
}

static void heapify_down(pqueue* pq, int index){
    if(pq == NULL) return;
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < pq->count
        //&& pq->elements[left].priority < pq->elements[smallest].priority)
        && pq->comparator(pq->elements[left].priority,pq->elements[smallest].priority))
        smallest = left;

    if (right < pq->count
        //&& pq->elements[right].priority < pq->elements[smallest].priority)
        && pq->comparator(pq->elements[right].priority,pq->elements[smallest].priority))
        smallest = right;

    if (smallest != index) {
        swap(&pq->elements[index], &pq->elements[smallest]);
        heapify_down(pq, smallest);
    }
}

pq_state pq_create(int capacity, pq_compare_fn comparator, pqueue** out_result){
    if(capacity <= 0) return PQ_ERROR_INVALID_ARGUMENT;
    pqueue* pq = malloc(sizeof(pqueue));
    if(pq == NULL) return PQ_ERROR_OUT_OF_MEMORY;
    pq->capacity = capacity; 
    pq->count = 0;
    pq->comparator = comparator;
    pq->elements = calloc(capacity, sizeof(pq_element));
    if(pq->elements == NULL){
        free(pq);
        return PQ_ERROR_OUT_OF_MEMORY;
    }    
    *out_result = pq;
    return PQ_OK;

}

void pq_destroy(pqueue* pq){
    if (pq == NULL) return; 
    if (pq->elements !=NULL) free(pq->elements);
    free(pq);
}


pq_state pq_enqueue(pqueue* pq, float priority, void* content){
    if (pq == NULL || isnan(priority)) return PQ_ERROR_INVALID_ARGUMENT; 
    if (pq->capacity == pq->count) return PQ_ERROR_FULL;
    //creating element
    pq_element new_pq_el = {.content = content, .priority = priority};
    //enqueueing
    pq->elements[pq->count++]=new_pq_el;
    heapify_up(pq,pq->count-1);
    return PQ_OK;
}

bool pq_dequeue(pqueue* pq, pq_element* out_item){
    if(pq == NULL || pq->count == 0 || out_item == NULL) return false;

    pq_element pq_el = pq->elements[0];
    pq->elements[0] = pq->elements[--pq->count];
    heapify_down(pq, 0);
    out_item->priority = pq_el.priority;
    out_item->content = pq_el.content;
    return true;
}

bool pq_peek(const pqueue* pq, pq_element* out_item){
    if(pq == NULL || pq->count == 0 || out_item == NULL) return false;
    *out_item = pq->elements[0];
    return true;
}

bool pq_get_count(const pqueue* pq, int* out_result){
    if(pq == NULL) return false;
    *out_result = pq->count;
    return true;
}

bool pq_get_capacity(const pqueue* pq, int* out_result){
    if(pq == NULL) return false;
    *out_result = pq->capacity;
    return true;
}

bool pq_clear(pqueue* pq){
    if(pq == NULL) return false;
    pq->count=0;
    return true;
}
