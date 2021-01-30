//
// Created by dbyard on 02/12/2020.
//

#ifndef JETSAM_HEAP_H
#define JETSAM_HEAP_H

#include <stdlib.h>
#include <stddef.h>

/**
 * Given any size_t of memory, realign it to the processor's maximum alignment.
 */
#define realign(memory) \
     ((memory) + (sizeof(max_align_t) - 1)) & ~(sizeof(max_align_t) - 1)

/**
 * Initialize the memory heap.
 * @param size minimum size of the memory heap.
 */
void g_heap_init();

/**
 * Return a unique pointer from the memory heap.
 * @param size the minimum size to allocate.
 * @return an aligned pointer to at least the number of bytes requested, or NULL if out of heap space.
 */
void* g_heap_allocate(size_t size);

/**
 * Drop in replacement for malloc() using the memory heap.
 */
void* g_heap_emulate_malloc(size_t size);

/**
 * Drop in replacement for calloc() using the memory heap.
 */
void* g_heap_emulate_calloc(size_t nmemb, size_t size);

/**
 * Drop in replacement for realloc() using the memory heap.
 */
void* g_heap_emulate_realloc(void* ptr, size_t size);

/**
 * Drop in replacement for reallocarray() using the memory heap.
 */
void* g_heap_emulate_reallocarray(void* ptr, size_t nmemb, size_t size);

/**
 * Drop in replacement for free() using the memory heap.
 */
void g_heap_emulate_free(void* ptr);

/**
 * Destroy the memory heap.
 */
void g_heap_destroy();

#endif //JETSAM_HEAP_H
