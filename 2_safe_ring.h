#ifndef ERING_H_
#define ERING_H_

#include <stdlib.h>

typedef struct {
    unsigned int capacity;
    void **ring;
    unsigned int push_cursor;
    unsigned int pop_cursor;
} Ering;

int ering_init(Ering *const ring, unsigned int capacity)
{
    if (!ring)
        return 0;
    ring->capacity = capacity;
    ring->push_cursor = 0;
    ring->pop_cursor = 0;
    ring->ring = malloc(ring->capacity * sizeof(void *));
    if (!ring->ring)
        return 0;
    return 1;
}

Ering *ering_new(unsigned int capacity)
{
    Ering *ring = malloc(sizeof(Ering) + capacity * sizeof(void *));
    if (!ring)
        return NULL;
    ring->capacity = capacity;
    ring->push_cursor = 0;
    ring->pop_cursor = 0;
    ring->ring = (void **)((char *)ring + sizeof(Ering));
    return ring;
}

void ering_release(Ering *const ring)
{
    if (!ring)
        return;
    if (ring->ring)
        free(ring->ring);
    ring->push_cursor = 0;
    ring->pop_cursor = 0;
}

int ering_push(Ering *const ring, void *const value)
{
    unsigned int pop_c;
    __atomic_load(&ring->pop_cursor, &pop_c, __ATOMIC_ACQUIRE);

    if ((ring->push_cursor - pop_c) == ring->capacity)
        return 0;
    ring->ring[ring->push_cursor % ring->capacity] = value;
    __atomic_fetch_add(&ring->push_cursor, 1, __ATOMIC_RELEASE);
    return 1;
}

int ering_pop(Ering *const ring, void **const value)
{
    unsigned int push_c;
    __atomic_load(&ring->push_cursor, &push_c, __ATOMIC_ACQUIRE);
    if ((push_c - ring->pop_cursor) == 0)
        return 0;
    *value = ring->ring[ring->pop_cursor % ring->capacity];
    __atomic_fetch_add(&ring->pop_cursor, 1, __ATOMIC_RELEASE);
    return 1;
}

#endif
