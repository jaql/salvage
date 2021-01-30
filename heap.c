#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "heap.h"
#include "log.h"
#include "opts.h"

/**
 * Very basic heap.
 */
struct Heap {
    /**
     * Lock for accessing the heap.
     */
    pthread_mutex_t lock;

    /**
     * Total size in bytes of the heap.
     */
    volatile size_t size;

    /**
     * Total bytes consumed of the heap.
     */
    volatile size_t used;

    /**
     * Pointer to start of heap memory.
     */
    volatile char* memory;
};

/**
 * The heap instance.
 */
struct Heap g_heap_instance;

/**
 * Pointer to the heap instance.  In future we might allocate this differently.
 */
struct Heap* g_heap = &g_heap_instance;

void g_heap_init() {
    int error_code;
    char* memory;
    int realigned_size;

    TRACE("g_heap_init()");

    realigned_size = realign(g_opts->heap_size);
    TRACEV("realigned to %d", realigned_size);

    error_code = pthread_mutex_init(&g_heap->lock, NULL);
    if (error_code != 0) {
        FATALV(FATAL_ERROR_HEAP_MUTEX_INIT, "failed mutex init with code %d", error_code);
    }
    INFO("mutex initialized");

    memory = malloc(realigned_size);
    if (memory == NULL) {
        FATALV(FATAL_ERROR_HEAP_MALLOC, "failed malloc of %d bytes", realigned_size);
    }
    INFOV("%d bytes allocated at %p", realigned_size, memory);

    error_code = mlock(memory, realigned_size);
    if (error_code == -1) {
        FATALV(FATAL_ERROR_HEAP_MALLOC, "failed mlock of %d bytes: %s", realigned_size, strerror(errno));
    }
    INFOV("%d bytes locked at %p", realigned_size, memory);

    g_heap->size = realigned_size;
    g_heap->memory = memory;
    g_heap->used = 0;
}

void* g_heap_allocate(size_t size) {
    #define UNLOCK_HEAP_MUTEX \
        error_code = pthread_mutex_unlock(&g_heap->lock); \
        if (error_code != 0) { \
            ERRORV("Mutex unlock failed with code %d, expect deadlock", error_code); \
        }

    void* result;
    int error_code;
    int realigned_size;

    MEMLOGV("g_heap_allocate(%d)", size);

    error_code = pthread_mutex_lock(&g_heap->lock);
    if (error_code != 0) {
        ERRORV("Mutex lock failed with code %d", error_code);
        return NULL;
    }

    realigned_size = realign(size);

    MEMLOGV("%d realigned to %d", size, realigned_size);

    if (g_heap->used + realigned_size >= g_heap->size) {
        ERRORV("heap exhausted by %d", g_heap->used + realigned_size - realigned_size);
        UNLOCK_HEAP_MUTEX;
        return NULL;
    }

    result = &g_heap->memory[g_heap->used];
    g_heap->used += realigned_size;

    MEMLOGV("Now used %d bytes of heap", g_heap->used);

    UNLOCK_HEAP_MUTEX;

    MEMLOGV("g_heap_alloc(%d) -> %p", size, result);
    return result;
}

void* g_heap_emulate_malloc(size_t size) {
    int realigned_size;
    void* result;

    MEMLOGV("g_heap_emulate_malloc(%ld)", size);

    if (size == 0) {
        return NULL;
    }

    realigned_size = realign(size);
    result = g_heap_allocate(realigned_size);

    MEMLOGV("curl_malloc_callback_fn(%ld) -> %p", size, result);
    return result;
}

void* g_heap_emulate_realloc(void* ptr, size_t size) {
    int realigned_size;
    void* result;

    MEMLOGV("g_heap_emulate_realloc(%p, %ld)", ptr, size);

    if (ptr == NULL || size == 0) {
        return g_heap_emulate_malloc(size);
    }

    realigned_size = size;
    result = g_heap_allocate(realigned_size);
    memcpy(result, ptr, realigned_size);

    MEMLOGV("g_heap_emulate_realloc(%p, %ld) -> %p", ptr, size, result);
    return result;
}

void* g_heap_emulate_calloc(size_t nmemb, size_t size) {
    int realigned_size;
    void* result;

    MEMLOGV("g_heap_emulate_calloc(%ld, %ld)", nmemb, size);

    realigned_size = realign(size);
    MEMLOGV("%d realigned to %d", size, realigned_size);

    result = g_heap_allocate(nmemb * realigned_size);

    MEMLOGV("g_heap_emulate_calloc(%ld, %ld) -> %p", nmemb, size, result);
    return result;
}

void* g_heap_emulate_reallocarray(void* ptr, size_t nmemb, size_t size) {
    int realigned_size;
    void* result;

    MEMLOGV("g_heap_emulate_reallocarray(%p, %ld)", ptr, size);

    if (size < 1) {
        return NULL;
    }

    realigned_size = realign(size);

    result = g_heap_allocate(nmemb * realigned_size);
    memcpy(result, ptr, nmemb * realigned_size);

    MEMLOGV("g_heap_emulate_reallocarray(%p, %ld) -> %p", ptr, size, result);
    return result;
}

void g_heap_emulate_free(void* ptr) {  // Stubbed.  We do not actually free anything in MVP.
    MEMLOGV("g_heap_emulate_free(%p)", ptr);
}

void g_heap_destroy() {
    int error_code;
    TRACE("g_heap_destroy()");

    error_code = pthread_mutex_destroy(&g_heap->lock);
    if (error_code != 0) {
        ERRORV("failed mutex destroy with code %d", error_code);
    }

    error_code = munlock(g_heap->memory, g_heap->size);
    if (error_code != 0) {
        ERRORV("failed munlock of %d bytes at %p: %s", g_heap->size, g_heap->memory, strerror(errno));
    }

    free(g_heap->memory);

    g_heap->size = 0;
    g_heap->memory = NULL;
    g_heap->used = 0;
}
