#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sched.h>
#include <stdatomic.h>

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

#ifdef NDEBUG
#define TEST_SIZE 400000000
#define WARM_SIZE 40000
#define BUFFER_SIZE 1024
#else
#define TEST_SIZE 128
#define WARM_SIZE 16
#define BUFFER_SIZE 8
#endif

int cond = 0;
volatile atomic_int trigger = 0;

int values[16];
int result[16];

struct timespec start, end;

int bind_cpu(int core)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(core, &mask);
    return sched_setaffinity(0, sizeof(mask), &mask);
}

void *produce(void *const data) {
    if (bind_cpu(0) != 0) {
        printf("PRODUCER FAILED\n");
        pthread_exit(NULL);
    }

    Ering *const ring = (Ering *const)data;
    int idx = 0;

    for (unsigned int i = 0; i < WARM_SIZE;) {
        idx = i & 15;
        values[idx] = 10000 + i;
        if (ering_push(ring, &values[idx]))
            ++i;
    }

    while (!trigger) {
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (unsigned int i = 0; i < TEST_SIZE;) {
        idx = i & 15;
        values[idx] = 10000 + i;
        if (ering_push(ring, &values[idx]))
            ++i;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    pthread_exit(NULL);
}


void *consume(void *const data) {
    if (bind_cpu(2) != 0) {
        printf("CONSUMER FAILED\n");
        pthread_exit(NULL);
    }

    Ering *const ring = (Ering *const)data;
    int* pointer = NULL;
    int idx = 0;

    for (unsigned int i = 0; i < WARM_SIZE;) {
        if (ering_pop(ring, (void**)&pointer)) {
            idx = i & 15;
            result[idx] = *pointer;
            ++i;
        }
    }

    trigger = 1;

    for (unsigned int i = 0; i < TEST_SIZE;) {
        if (ering_pop(ring, (void**)&pointer)) {
            idx = i & 15;
            result[idx] = *pointer;
            ++i;
        }
    }

    pthread_exit(NULL);
}

Ering ring;

int main() {
    //Ering* r = ering_new(BUFFER_SIZE);
    Ering* r = &ring;

    ering_init(r, BUFFER_SIZE);

    pthread_t producer, consumer;
    
    pthread_create(&producer, NULL, &produce, r);
    pthread_create(&consumer, NULL, &consume, r);
    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);

    double execution_sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double operations_per_sec = TEST_SIZE / execution_sec;
    printf("Execution time: %f seconds\n", execution_sec);
    printf("Operations per second: %.2f\n", operations_per_sec);

    ering_release(r);
    //free(r);
}
