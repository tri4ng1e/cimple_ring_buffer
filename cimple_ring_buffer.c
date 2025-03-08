#include "cimple_ring_buffer.h"
#include <stdlib.h>
#include <string.h>

// Internal helper for index arithmetic: if power-of-two optimization is enabled,
// use bitmasking, otherwise use modulo.
static inline size_t ring_buffer_index(const ring_buffer_t *rb, size_t index) {
#if CIMPLE_RING_BUFFER_POWER_OF_TWO_OPTIMIZATION
    return index & (rb->capacity - 1);
#else
    return index % rb->capacity;
#endif
}

bool ring_buffer_init(ring_buffer_t *rb, size_t capacity, size_t item_size) {
    if (!rb || capacity == 0 || item_size == 0) return false;
#if CIMPLE_RING_BUFFER_POWER_OF_TWO_OPTIMIZATION
    // Ensure capacity is a power of two for optimization.
    if ((capacity & (capacity - 1)) != 0) return false;
#endif
    rb->buffer = malloc(capacity * item_size);
    if (!rb->buffer) return false;

    rb->capacity = capacity;
    rb->item_size = item_size;
    rb->count = 0;
    rb->head = 0;
    rb->tail = 0;
    return true;
}

void ring_buffer_free(ring_buffer_t *rb) {
    if (rb) {
        free(rb->buffer);
        rb->buffer = NULL;
    }
}

void ring_buffer_clear(ring_buffer_t *rb) {
    if (rb) {
        rb->count = 0;
        rb->head = 0;
        rb->tail = 0;
    }
}

bool ring_buffer_push(ring_buffer_t *rb, const void *item) {
    if (!rb || !item) return false;

    if (ring_buffer_is_full(rb)) {
#if CIMPLE_RING_BUFFER_ALLOW_OVERWRITE
        // Overwrite mode: advance tail to drop the oldest item.
        rb->tail = ring_buffer_index(rb, rb->tail + 1);
#else
        // Non-overwrite mode: push fails if full.
        return false;
#endif
    } else {
        rb->count++;
    }

    memcpy((char *)rb->buffer + rb->head * rb->item_size, item, rb->item_size);
    rb->head = ring_buffer_index(rb, rb->head + 1);
    return true;
}

void *ring_buffer_next_slot(ring_buffer_t *rb) {
    if (!rb) return NULL;

    void *next_space = (char *)rb->buffer + rb->head * rb->item_size;

    if (ring_buffer_is_full(rb)) {
        // Overwrite the oldest item.
        rb->tail = ring_buffer_index(rb, rb->tail + 1);
    } else {
        rb->count++;
    }
    rb->head = ring_buffer_index(rb, rb->head + 1);

    return next_space;
}

bool ring_buffer_pop(ring_buffer_t *rb, void *item) {
    if (!rb || !item || ring_buffer_is_empty(rb)) return false;

    memcpy(item, (char *)rb->buffer + rb->tail * rb->item_size, rb->item_size);
    rb->tail = ring_buffer_index(rb, rb->tail + 1);
    rb->count--;
    return true;
}

bool ring_buffer_remove(ring_buffer_t *rb, size_t index) {
    if (!rb || index >= rb->count) return false;

    size_t actual_index = ring_buffer_index(rb, rb->tail + index);
#if CIMPLE_RING_BUFFER_PRESERVE_ORDER
    // Shift items to preserve order.
    for (size_t i = index; i < rb->count - 1; ++i) {
        size_t from_index = ring_buffer_index(rb, rb->tail + i + 1);
        size_t to_index = ring_buffer_index(rb, rb->tail + i);
        memcpy((char *)rb->buffer + to_index * rb->item_size,
               (char *)rb->buffer + from_index * rb->item_size,
               rb->item_size);
    }
    // Adjust head backwards.
    rb->head = ring_buffer_index(rb, rb->head + rb->capacity - 1);
#else
    // Fast removal: replace with last item.
    if (index < rb->count - 1) { // If not the last item, swap with the last.
        size_t last_index = ring_buffer_index(rb, rb->tail + rb->count - 1);
        memcpy((char *)rb->buffer + actual_index * rb->item_size,
               (char *)rb->buffer + last_index * rb->item_size,
               rb->item_size);
    }
    rb->head = ring_buffer_index(rb, rb->head + rb->capacity - 1);
#endif
    rb->count--;
    return true;
}

size_t ring_buffer_remove_all(ring_buffer_t *rb, bool (*predicate)(const void* item, void* context), void *context) {
    if (!rb || !predicate) return 0;

    size_t keepCount = 0; // Count of items to be kept.
    size_t originalCount = rb->count;

    // Iterate over all items.
    for (size_t i = 0; i < originalCount; ++i) {
        void *item_ptr = ring_buffer_get(rb, i);
        if (!predicate(item_ptr, context)) {
            // Move item if needed.
            if (i != keepCount) {
                void *targetSlot = (char *)rb->buffer + ring_buffer_index(rb, rb->tail + keepCount) * rb->item_size;
                memcpy(targetSlot, item_ptr, rb->item_size);
            }
            keepCount++;
        }
    }

    rb->count = keepCount;
    rb->head = ring_buffer_index(rb, rb->tail + rb->count);
    return originalCount - keepCount;
}

void *ring_buffer_get(ring_buffer_t *rb, size_t logical_index) {
    if (!rb || logical_index >= rb->count) return NULL;
    size_t actual_index = ring_buffer_index(rb, rb->tail + logical_index);
    return (char *)rb->buffer + actual_index * rb->item_size;
}

ring_buffer_item_t *ring_buffer_find(ring_buffer_t *rb, bool (*predicate)(const void* item, void* context), void *context) {
    if (!rb || !predicate) return NULL;

    for (size_t i = 0; i < rb->count; ++i) {
        void *item_ptr = ring_buffer_get(rb, i);
        if (predicate(item_ptr, context)) {
            ring_buffer_item_t *pair = malloc(sizeof(ring_buffer_item_t)); // Note: dynamic allocation may be avoided in embedded.
            if (pair) {
                pair->item = item_ptr;
                pair->index = i;
            }
            return pair;
        }
    }
    return NULL;
}

ring_buffer_item_t *ring_buffer_find_all(ring_buffer_t *rb, bool (*predicate)(const void* item, void* context), void *context, size_t *count) {
    if (!rb || !predicate || !count) return NULL;

    ring_buffer_item_t *matches = malloc(rb->count * sizeof(ring_buffer_item_t));
    if (!matches) return NULL;

    size_t matchesCount = 0;
    for (size_t i = 0; i < rb->count; ++i) {
        void *item_ptr = ring_buffer_get(rb, i);
        if (predicate(item_ptr, context)) {
            matches[matchesCount].item = item_ptr;
            matches[matchesCount].index = i;
            matchesCount++;
        }
    }

    if (matchesCount == 0) {
        free(matches);
        *count = 0;
        return NULL;
    } else {
        // Shrink allocation to actual number of matches.
        ring_buffer_item_t *resized = realloc(matches, matchesCount * sizeof(ring_buffer_item_t));
        *count = matchesCount;
        return resized ? resized : matches;
    }
}

void ring_buffer_iterate(ring_buffer_t *rb, void (*func)(ring_buffer_item_t*, void*), void *context) {
    if (!rb || !func) return;

    ring_buffer_item_t item;
    for (size_t i = 0; i < rb->count; ++i) {
        item.index = i;
        item.item  = ring_buffer_get(rb, i);
        func(&item, context);
    }
}

void *ring_buffer_to_linear_array(ring_buffer_t *rb) {
    if (!rb) return NULL;

    void *array = malloc(rb->count * rb->item_size);
    if (!array) return NULL;

    for (size_t i = 0, idx = rb->tail; i < rb->count; ++i) {
        memcpy((char *)array + i * rb->item_size,
               (char *)rb->buffer + ring_buffer_index(rb, idx) * rb->item_size,
               rb->item_size);
        idx++;
    }
    return array;
}
