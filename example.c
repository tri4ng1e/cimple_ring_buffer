#include "cimple_ring_buffer.h"
#include <stdlib.h>
#include <stdio.h>

// Corrected iterate function: properly dereference the item member.
void process_item(ring_buffer_item_t *item, void* context) {
    int value = *(int *)item->item;
    printf("  item %zu: %d\n", item->index, value);
}

bool is_even(const void *item, void *context) {
    return (*(const int *)item % 2) == 0;
}

bool find_item(const void *item, void *context) {
    return *(const int *)item == *(const int *)context;
}

int main() {
    ring_buffer_t rb;
    size_t capacity = 8;  // Choose a small power-of-two capacity for efficiency

    printf("Initializing ring buffer with capacity: %zu\n", capacity);
    if (!ring_buffer_init(&rb, capacity, sizeof(int))) {
        printf("Failed to initialize the ring buffer.\n");
        return -1;
    }

    printf("Pushing items into the ring buffer:\n");
    for (int i = 0; i < 8; ++i) {
        int item = i + 1;
        if (!ring_buffer_push(&rb, &item)) {
            printf("Push failed at iteration %d (buffer full without overwrite mode)\n", i);
        }
        printf("  pushed: %d\n", item);
    }

    // Demonstrate next_slot usage.
    printf("Using next_slot for in-place update:\n");
    for (int i = 8; i < 12; ++i) {
        int *item_ptr = ring_buffer_next_slot(&rb);
        *item_ptr = i + 1;
        printf("  next_slot assigned: %d\n", *item_ptr);
    }

    // Direct access.
    printf("Direct access by logical index:\n");
    for (size_t i = 0; i < rb.count; ++i) {
        int *item_ptr = ring_buffer_get(&rb, i);
        printf("  item %zu: %d\n", i, *item_ptr);
    }

    // Iterating over items.
    printf("Iterating over items:\n");
    ring_buffer_iterate(&rb, process_item, NULL);

    // Finding an item.
    int search_value = 42;
    printf("Finding value %d... ", search_value);
    ring_buffer_item_t *found_item = ring_buffer_find(&rb, find_item, &search_value);
    if (found_item == NULL) {
        printf("Not found.\n");
    } else {
        printf("Found at logical index %zu.\n", found_item->index);
        free(found_item);
    }

    // Removing an item.
    search_value = 5;
    printf("Finding value %d to remove... ", search_value);
    found_item = ring_buffer_find(&rb, find_item, &search_value);
    if (found_item != NULL) {
        printf("Found at index %zu. Removing it...\n", found_item->index);
        ring_buffer_remove(&rb, found_item->index);
        free(found_item);
    } else {
        printf("Not found.\n");
    }

    // Convert to linear array.
    int *linear_array = (int *)ring_buffer_to_linear_array(&rb);
    printf("Items from linear array:\n");
    for (size_t i = 0; i < rb.count; ++i) {
        printf("  item %zu: %d\n", i, linear_array[i]);
    }
    free(linear_array);

    // Find and remove all even numbers.
    size_t found_count;
    ring_buffer_item_t *found_items = ring_buffer_find_all(&rb, is_even, NULL, &found_count);
    printf("Found %zu even numbers.\n", found_count);
    if (found_items) {
        for (size_t i = 0; i < found_count; ++i) {
            printf("  even item at index %zu: %d\n", found_items[i].index, *(int*)(found_items[i].item));
        }
        free(found_items);
    }

    printf("Removing all even numbers... ");
    size_t removed_count = ring_buffer_remove_all(&rb, is_even, NULL);
    printf("%zu element(s) removed.\n", removed_count);

    printf("Final buffer iteration:\n");
    ring_buffer_iterate(&rb, process_item, NULL);

    ring_buffer_free(&rb);
    return 0;
}
