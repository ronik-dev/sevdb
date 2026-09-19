#include "pqueue.h"
#include <stdlib.h>
#include <math.h>

struct pqueue {
    int capacity; 
    int count;
    pq_element *elements;
};

static void swap(pq_element* pq_el1, pq_element* pq_el2){
    pq_element temp = *pq_el1;
    *pq_el1 = *pq_el2;
    *pq_el2 = temp;
}

static void heapify_up(pqueue* pq, int index){
    if(index > 0
       && pq->elements[(index - 1) / 2].priority > pq->elements[index].priority) {
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
        && pq->elements[left].priority < pq->elements[smallest].priority)
        smallest = left;

    if (right < pq->count
        && pq->elements[right].priority < pq->elements[smallest].priority)
        smallest = right;

    if (smallest != index) {
        swap(&pq->elements[index], &pq->elements[smallest]);
        heapify_down(pq, smallest);
    }
}

pqueue* pq_create(int capacity){
    if(capacity <= 0) return NULL;
    pqueue* pq = malloc(sizeof(pqueue));
    if(pq == NULL) return NULL;
    pq->capacity = capacity; 
    pq->count = 0;
    pq->elements = calloc(capacity, sizeof(pq_element));
    if(pq->elements == NULL){
        free(pq);
        return NULL;
    }    
    return pq;
}

void pq_destroy(pqueue* pq){
    if(pq == NULL) return; 
    free(pq->elements);
    free(pq);
}


bool pq_enqueue(pqueue* pq, float priority, void* content){
    if(pq == NULL || pq->capacity == pq->count || isnan(priority)) return false;
    //creating element
    pq_element new_pq_el = {.content = content, .priority = priority};
    //enqueueing
    pq->elements[pq->count++]=new_pq_el;
    heapify_up(pq,pq->count-1);
    return true;
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

int pq_get_count(const pqueue* pq){
    if(pq == NULL) return -1;
    return pq->count;
}

int pq_get_capacity(const pqueue* pq){
    if(pq == NULL) return -1;
    return pq->capacity;
}

bool pq_clear(pqueue* pq){
    if(pq == NULL) return false;
    pq->count=0;
    return true;
}
