#ifndef ERING_H_
#define ERING_H_

#include <stdlib.h>

#define CACHE_LINE_SIZE 64

static inline unsigned int align32pow2(unsigned int x)
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;

    return x + 1;
}

typedef struct {
    __attribute__((aligned(CACHE_LINE_SIZE))) unsigned int cached_push_cursor;
    __attribute__((aligned(CACHE_LINE_SIZE))) unsigned int cached_pop_cursor;
    __attribute__((aligned(CACHE_LINE_SIZE))) unsigned int push_cursor;
    __attribute__((aligned(CACHE_LINE_SIZE))) unsigned int pop_cursor;

    unsigned int capacity;
    unsigned int mask;
    void** ring;
} __attribute__((aligned(CACHE_LINE_SIZE))) Ering;

int ering_init(Ering *const ring, unsigned int capacity) {
    if (!ring)
        return 0;
    ring->capacity = align32pow2(capacity);
    ring->mask = ring->capacity - 1;
    ring->push_cursor = 0;
    ring->pop_cursor = 0;
    ring->cached_push_cursor = 0;
    ring->cached_pop_cursor = 0;
    ring->ring = malloc(ring->capacity * sizeof(void *));
    if (!ring->ring)
        return 0;
    return 1;
}

Ering* ering_new(unsigned int capacity) {
    unsigned int cap = align32pow2(capacity);
    Ering* ring = malloc(sizeof(Ering) + cap * sizeof(void *));
    if (!ring)
        return NULL;
    ring->capacity = cap;
    ring->mask = cap - 1;
    ring->push_cursor = 0;
    ring->pop_cursor = 0;
    ring->cached_push_cursor = 0;
    ring->cached_pop_cursor = 0;
    ring->ring = (void**)((char*)ring + sizeof(Ering));
    return ring;
}

void ering_release(Ering *const ring) {
    if (!ring)
        return;
    if (ring->ring)
        free(ring->ring);
    ring->push_cursor = 0;
    ring->pop_cursor = 0;
}

int ering_push(Ering *const ring, void *const value) {
    if (ring->push_cursor - ring->cached_pop_cursor == ring->capacity) {
        __atomic_load(&ring->pop_cursor, &ring->cached_pop_cursor,  __ATOMIC_ACQUIRE);
        if (ring->push_cursor - ring->cached_pop_cursor == ring->capacity) {
            return 0;
        }
    }
    ring->ring[ring->push_cursor & ring->mask] = value;
    __atomic_fetch_add(&ring->push_cursor, 1, __ATOMIC_RELEASE);
    return 1;
}

int ering_pop(Ering *const ring, void **const value) {
    if (ring->cached_push_cursor - ring->pop_cursor == 0) {
        __atomic_load(&ring->push_cursor, &ring->cached_push_cursor,  __ATOMIC_ACQUIRE);
        if (ring->cached_push_cursor - ring->pop_cursor == 0) {
            return 0;
        }
    }
    *value = ring->ring[ring->pop_cursor & ring->mask];
    __atomic_fetch_add(&ring->pop_cursor, 1, __ATOMIC_RELEASE);
    return 1;
}

#endif
