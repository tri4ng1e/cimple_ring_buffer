#include "cimple_ring_buffer.h"
#include <stdlib.h>
#include <string.h>

bool ring_buffer_init(ring_buffer_t *rb, size_t capacity, size_t item_size) {
    rb->buffer = malloc(capacity * item_size);
    if (!rb->buffer) return false;

    rb->capacity = capacity;
    rb->count = 0;
    rb->head = 0;
    rb->tail = 0;
    rb->item_size = item_size;
    return true;
}

void ring_buffer_free(ring_buffer_t *rb) {
    free(rb->buffer);
}

bool ring_buffer_push(ring_buffer_t *rb, const void *item) {
    size_t next_head = (rb->head + 1) % rb->capacity;
    if (rb->count == rb->capacity) {
        rb->tail = (rb->tail + 1) % rb->capacity; // Move tail if buffer is full
    } else {
        rb->count++;
    }

    memcpy((char *)rb->buffer + rb->head * rb->item_size, item, rb->item_size);
    rb->head = next_head;
    return true;
}

void *ring_buffer_next_slot(ring_buffer_t *rb) {
    void *next_space = (char *)rb->buffer + rb->head * rb->item_size;

    // if the buffer is full, advance the tail to overwrite the oldest data
    if (rb->count == rb->capacity) {
        rb->tail = (rb->tail + 1) % rb->capacity;
    } else {
        rb->count++; // increase count if not overwriting
    }

    // always advance the head
    rb->head = (rb->head + 1) % rb->capacity;

    return next_space;
}

bool ring_buffer_pop(ring_buffer_t *rb, void *item) {
    if (rb->count == 0) return false;

    memcpy(item, (char *)rb->buffer + rb->tail * rb->item_size, rb->item_size);
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count--;
    return true;
}

bool ring_buffer_remove(ring_buffer_t *rb, size_t index) {
    if (index >= rb->count) return false;

#ifdef CIMPLE_RING_BUFFER_PRESERVE_ORDER
    // Preserve order: shift all elements down after the removed item
    size_t actual_index = (rb->tail + index) % rb->capacity;
    for (size_t i = index; i < rb->count - 1; ++i) {
        size_t next_index = (actual_index + 1) % rb->capacity;
        memcpy((char *)rb->buffer + actual_index * rb->item_size, (char *)rb->buffer + next_index * rb->item_size, rb->item_size);
        actual_index = next_index;
    }
    rb->head = (rb->head + rb->capacity - 1) % rb->capacity; // Adjust head backwards
#else
    // Optimization: Swap with the last item to avoid shifting, if order preservation is not required
    if (index < rb->count - 1) { // No action needed if removing the last item
        size_t last_index = (rb->tail + rb->count - 1) % rb->capacity;
        size_t remove_index = (rb->tail + index) % rb->capacity;
        memcpy((char *)rb->buffer + remove_index * rb->item_size, (char *)rb->buffer + last_index * rb->item_size, rb->item_size);
    }
    rb->head = (rb->head + rb->capacity - 1) % rb->capacity; // Adjust head backwards
#endif

    rb->count--;
    return true;
}


void *ring_buffer_get(ring_buffer_t *rb, size_t logical_index) {
    if (logical_index >= rb->count) return NULL;
    size_t actual_index = (rb->tail + logical_index) % rb->capacity;
    return (char *)rb->buffer + actual_index * rb->item_size;
}

ring_buffer_item_t *ring_buffer_find(ring_buffer_t *rb, bool (*predicate)(const void* item, void* context), void *context) {
    for (size_t i = 0; i < rb->count; ++i) {
        void *item = ring_buffer_get(rb, i);
        if (predicate(item, context)) {
            ring_buffer_item_t *pair = malloc(sizeof(ring_buffer_item_t)); // Allocate a new ItemIndexPair
            pair->item = item;
            pair->index = i;
            return pair; // Return the pair if a match is found
        }
    }
    return NULL; // Return NULL if no match is found
}

ring_buffer_item_t *ring_buffer_find_all(ring_buffer_t *rb, bool (*predicate)(const void* item, void* context), void *context, size_t *count) {
    ring_buffer_item_t *matches = malloc(rb->count * sizeof(ring_buffer_item_t));
    size_t matchesCount = 0;

    for (size_t i = 0; i < rb->count; ++i) {
        void *item = ring_buffer_get(rb, i);
        if (predicate(item, context)) {
            matches[matchesCount].item = item;
            matches[matchesCount].index = i;
            matchesCount++;
        }
    }

    if (matchesCount == 0) {
        free(matches);
        matches = NULL;
    } else {
        matches = realloc(matches, matchesCount * sizeof(ring_buffer_item_t));
    }

    *count = matchesCount;
    return matches;
}

void ring_buffer_iterate(ring_buffer_t *rb, void (*func)(ring_buffer_item_t*, void*), void *context) {
    ring_buffer_item_t item;
    for (size_t i = 0; i < rb->count; ++i) {
        item.index = i;
        item.item  = ring_buffer_get(rb, i);
        func(&item, context);
    }
}

void *ring_buffer_to_linear_array(ring_buffer_t *rb) {
    void *array = malloc(rb->count * rb->item_size);
    if (!array) return NULL;

    for (size_t i = 0, idx = rb->tail; i < rb->count; ++i) {
        memcpy((char *)array + i * rb->item_size, (char *)rb->buffer + idx * rb->item_size, rb->item_size);
        idx = (idx + 1) % rb->capacity;
    }
    return array;
}
