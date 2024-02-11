#pragma once

// #define CIMPLE_RING_BUFFER_PRESERVE_ORDER

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    void *buffer;     // pointer to the buffer holding the data
    size_t capacity;  // maximum number of items in the buffer
    size_t count;     // current number of items in the buffer
    size_t head;      // points to the next insertion point
    size_t tail;      // points to the oldest item
    size_t item_size; // size of each item
} ring_buffer_t;

typedef struct {
    void *item;   // pointer to the item
    size_t index; // logical index of the item within the ring buffer
} ring_buffer_item_t;

bool  ring_buffer_init  (ring_buffer_t *rb, size_t capacity, size_t item_size);
void  ring_buffer_free  (ring_buffer_t *rb);

bool  ring_buffer_push  (ring_buffer_t *rb, const void *item);
bool  ring_buffer_pop   (ring_buffer_t *rb, void *item);
bool  ring_buffer_remove(ring_buffer_t *rb, size_t index);

void *ring_buffer_next_slot(ring_buffer_t *rb);

void *ring_buffer_get   (ring_buffer_t *rb, size_t logical_index);

ring_buffer_item_t *ring_buffer_find    (ring_buffer_t *rb, bool (*predicate)(const void* item, void* context), void *context);
ring_buffer_item_t *ring_buffer_find_all(ring_buffer_t *rb, bool (*predicate)(const void* item, void* context), void *context, size_t *count);

void ring_buffer_iterate(ring_buffer_t *rb, void (*func)(ring_buffer_item_t*, void*), void *context);

void *ring_buffer_to_linear_array(ring_buffer_t *rb);
