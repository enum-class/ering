#include <assert.h>
#include <stdio.h>

#ifdef UNSAFE_RING
#include "1_unsafe_ring.h"
#elif defined(SAFE_RING)
#include "2_safe_ring.h"
#elif defined(FALSE_SHARING_RING)
#include "3_false_sharing_ring.h"
#elif defined(CACHE_RING)
#include "4_cache_ring.h"
#elif defined(MOD_RING)
#include "5_mod_ring.h"
#else
#error "A header should be defined"
#endif

int size(Ering *ring)
{
    return ring->push_cursor - ring->pop_cursor;
}

int is_empty(Ering *ring)
{
    return size(ring) == 0;
}

int is_full(Ering *ring)
{
    return (unsigned int)size(ring) == ring->capacity;
}

void test_init()
{
    Ering ring;

    assert(ering_init(&ring, 1024));
    assert(is_empty(&ring));
    assert(!is_full(&ring));

    ering_release(&ring);
}

void test_push_pop()
{
    Ering ring;
    int values[3] = { 1, 2, 3 };
    int *pointer = NULL;

    // push
    assert(ering_init(&ring, 3));
    assert(size(&ring) == 0);
    assert(is_empty(&ring));
    assert(!is_full(&ring));

    assert(ering_push(&ring, &values[0]));
    assert(size(&ring) == 1);
    assert(!is_empty(&ring));
    assert(!is_full(&ring));

    assert(ering_push(&ring, &values[1]));
    assert(size(&ring) == 2);
    assert(!is_empty(&ring));
    assert(!is_full(&ring));

    assert(ering_push(&ring, &values[2]));
    assert(size(&ring) == 3);
    assert(!is_empty(&ring));
#ifndef MOD_RING
    assert(is_full(&ring));
    assert(!ering_push(&ring, &values[0]));
#endif

    // pop
    assert(ering_pop(&ring, (void **)&pointer));
    assert(*pointer == 1);
    assert(size(&ring) == 2);
    assert(!is_empty(&ring));
    assert(!is_full(&ring));

    assert(ering_pop(&ring, (void **)&pointer));
    assert(*pointer == 2);
    assert(size(&ring) == 1);
    assert(!is_empty(&ring));
    assert(!is_full(&ring));

    assert(ering_pop(&ring, (void **)&pointer));
    assert(*pointer == 3);
    assert(size(&ring) == 0);
    assert(is_empty(&ring));
    assert(!is_full(&ring));

    assert(!ering_pop(&ring, (void **)&pointer));

    for (int i = 0; i < 10; ++i) {
        assert(ering_push(&ring, &values[i % 3]));
        assert(ering_pop(&ring, (void **)&pointer));
        assert(*pointer == values[i % 3]);
        assert(is_empty(&ring));
    }

    ering_release(&ring);
}

int main()
{
    test_init();
    test_push_pop();

#ifdef UNSAFE_RING
    printf("All unsafe tests passed\n");
#elif defined(SAFE_RING)
    printf("All safe tests passed\n");
#elif defined(FALSE_SHARING_RING)
    printf("All false sharing tests passed\n");
#elif defined(CACHE_RING)
    printf("All cache tests passed\n");
#elif defined(MOD_RING)
    printf("All mod tests passed\n");
#else
#error "A header should be defined"
#endif
    return 0;
}
