#include "cimple_ring_buffer.h"
#include <stdlib.h>
#include <stdio.h>

// example iterate function
void process_item(ring_buffer_item_t *item, void* context) {
    int value = *(int *)item;
    printf("  item %zu: %d\n", item->index, *(int*)item->item);
}

bool is_even(const void *item, void *context) {
    return (*(const int *)item % 2) == 0;
}

bool find_item(const void *item, void *context) {
    return *(const int *)item == *(const int *)context;
}

int main() {
    ring_buffer_t rb;
    size_t capacity = 10;

    printf("Ring buffer size: %zu.\n", capacity);

    if (!ring_buffer_init(&rb, capacity, sizeof(int))) {
        printf("Failed to initialize the ring buffer.\n");
        return -1;
    }

    printf("Creating items:\n");
    for (int i = 0; i < 10; ++i) {
        printf("  iteration %d: %d\n", i, i+1);
        int item = i + 1;
        ring_buffer_push(&rb, &item);
    }

    for (int i = 10; i < 20; ++i) {
        printf("  iteration %d: %d (next_slot)\n", i, i+1);
        int *item = ring_buffer_next_slot(&rb);
        *item = i + 1;
    }

    printf("Direct access by logical index:\n");
    for (size_t i = 0; i < rb.count; ++i) {
        int *item = ring_buffer_get(&rb, i);
        printf("  item %zu: %d\n", i, *item);
    }

    printf("Iterating over items:\n");
    ring_buffer_iterate(&rb, process_item, NULL);

    printf("Finding value 42... ");
    int search_value = 42;
    ring_buffer_item_t *found_item = ring_buffer_find(&rb, find_item, &search_value);
    if (found_item == NULL) {
        printf("Not found.\n");
    } else {
        free(found_item);
    }

    printf("Finding value 5... ");
    search_value = 15;
    found_item = ring_buffer_find(&rb, find_item, &search_value);
    if (found_item != NULL) {
        printf("Found at index %zu.\n", found_item->index);
        printf("  Removing it...\n");
        ring_buffer_remove(&rb, found_item->index);
        free(found_item);
    } else {
        printf("Not found.\n");
    }

    int *linear_array = (int *)ring_buffer_to_linear_array(&rb);
    printf("Processing items from linear array:\n");
    for (size_t i = 0; i < rb.count; ++i) {
        printf("  item %zu: %d\n", i, linear_array[i]);
    }
    free(linear_array);

    size_t found_count;
    ring_buffer_item_t *found_items = ring_buffer_find_all(&rb, is_even, NULL, &found_count);

    printf("Found %zu even numbers:\n", found_count);
    for (size_t i = 0; i < found_count; ++i) {
        printf("  item %zu: %d\n", found_items[i].index, *(int*)(found_items[i].item));
    }
    free(found_items);

    printf("Removeing all even numbers... ");
    size_t removed_count = ring_buffer_remove_all(&rb, is_even, NULL);
    printf("%zu element(s) removed.\n", removed_count);

    printf("Iterating over items:\n");
    ring_buffer_iterate(&rb, process_item, NULL);

    ring_buffer_free(&rb);

    return 0;
}
