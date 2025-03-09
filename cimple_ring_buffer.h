#pragma once

#include <stdbool.h>
#include <stddef.h>

// Define to preserve elements order during element removal.
// If set to 1 then removals shift items; if 0, the last item is swapped in.
#ifndef CIMPLE_RING_BUFFER_PRESERVE_ORDER
#define CIMPLE_RING_BUFFER_PRESERVE_ORDER 0
#endif

// Define to allow overwriting of oldest data when buffer is full.
// If set to 0, ring_buffer_push will fail when the buffer is full.
#ifndef CIMPLE_RING_BUFFER_ALLOW_OVERWRITE
#define CIMPLE_RING_BUFFER_ALLOW_OVERWRITE 1
#endif

// Define to optimize modulo operations when capacity is a power of two.
// When enabled, capacity must be a power of two.
#ifndef CIMPLE_RING_BUFFER_POWER_OF_TWO_OPTIMIZATION
#define CIMPLE_RING_BUFFER_POWER_OF_TWO_OPTIMIZATION 1
#endif

typedef struct {
    void *buffer;      // Pointer to the allocated buffer memory.
    size_t capacity;   // Maximum number of items in the buffer.
    size_t count;      // Current number of items stored.
    size_t head;       // Next insertion index.
    size_t tail;       // Index of the oldest item.
    size_t item_size;  // Size of each item in bytes.
} ring_buffer_t;

typedef struct {
    void *item;      // Pointer to the item.
    size_t index;    // Logical index of the item within the ring buffer.
} ring_buffer_item_t;

// Initializes the ring buffer (allocates memory).
// Returns true on success, false on failure.
bool ring_buffer_init(ring_buffer_t *rb, size_t capacity, size_t item_size);

// Initializes the ring buffer using a user-provided buffer.
bool ring_buffer_init_with_buffer(ring_buffer_t *rb, void *buffer, size_t capacity, size_t item_size);

// Frees any allocated memory for the ring buffer.
void ring_buffer_free(ring_buffer_t *rb);

// Resets the ring buffer to empty without freeing memory.
void ring_buffer_clear(ring_buffer_t *rb);

// Returns true if the ring buffer is empty.
static inline bool ring_buffer_is_empty(const ring_buffer_t *rb) {
    return rb && (rb->count == 0);
}

// Returns true if the ring buffer is full.
static inline bool ring_buffer_is_full(const ring_buffer_t *rb) {
    return rb && (rb->count == rb->capacity);
}

// Pushes an item onto the ring buffer.
// Returns true on success. If the buffer is full and overwrite is disabled, returns false.
bool ring_buffer_push(ring_buffer_t *rb, const void *item);

// Provides a pointer to the next slot for in-place writing.
// Overwrites oldest data if the buffer is full.
void *ring_buffer_next_slot(ring_buffer_t *rb);

// Pops (removes) the oldest item from the ring buffer and copies it into 'item'.
// Returns false if the buffer is empty.
bool ring_buffer_pop(ring_buffer_t *rb, void *item);

// Removes an item at the specified logical index.
// Returns false if the index is invalid.
bool ring_buffer_remove(ring_buffer_t *rb, size_t index);

// Removes all items matching a predicate.
// Returns the number of items removed.
size_t ring_buffer_remove_all(ring_buffer_t *rb, bool (*predicate)(const void* item, void* context), void *context);

// Returns a pointer to the item at the specified logical index, or NULL if index is invalid.
void *ring_buffer_get(ring_buffer_t *rb, size_t logical_index);

// Iterates over items in the buffer, calling the provided function for each item.
void ring_buffer_iterate(ring_buffer_t *rb, void (*func)(ring_buffer_item_t*, void*), void *context);

// Returns a linear copy of the ring buffer as a newly allocated array.
void *ring_buffer_to_linear_array(ring_buffer_t *rb);

// Returns the oldest item without removing it.
void *ring_buffer_peek(const ring_buffer_t *rb);

// Search functions (use dynamic memory allocation; in tight-memory embedded, consider alternatives).
ring_buffer_item_t *ring_buffer_find(ring_buffer_t *rb, bool (*predicate)(const void* item, void* context), void *context);
ring_buffer_item_t *ring_buffer_find_all(ring_buffer_t *rb, bool (*predicate)(const void* item, void* context), void *context, size_t *count);
