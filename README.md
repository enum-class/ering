# Ering
Ering is a step-by-step implementation of a lock-free, single-producer, single-consumer, fixed-size FIFO queue

## Step 1: Simple Unsafe Circular Ring
The first step implements a basic, non-thread-safe circular buffer in C.

<div align="center">
<img src="img/ring.jpg" alt="ring" width="350">
</div>

### Code
```c
typedef struct {
    unsigned int capacity;    // Maximum number of elements
    void** ring;              // Array of void pointers
    unsigned int push_cursor; // Index for next push
    unsigned int pop_cursor;  // Index for next pop
} Ering;

// Push an element into the ring
int ering_push(Ering *const ring, void* value) {
    if ((ring->push_cursor - ring->pop_cursor) == ring->capacity)
        return 0;
    ring->ring[ring->push_cursor % ring->capacity] = value;
    ++ring->push_cursor;
    return 1;
}

// Pop an element from the ring
int ering_pop(Ering *const ring, void** value) {
    if ((ring->push_cursor - ring->pop_cursor) == 0)
        return 0;
    *value = ring->ring[ring->pop_cursor % ring->capacity];
    ++ring->pop_cursor;
    return 1;
}
```

## Step 2: Adding Atomic Operations

In this step, I introduce atomic operations to make the FIFO queue safe for a single-producer, single-consumer (SPSC) scenario. By using atomic loads and fetches, I ensure that cursor updates are visible across threads without data races.

- I use `__atomic_load` with `__ATOMIC_ACQUIRE` to read the opposing cursor (**pop_cursor** in *ering_push*, **push_cursor** in *ering_pop*). This ensures the consumer or producer sees the latest value written by the other thread. Also `__atomic_fetch_add` with `__ATOMIC_RELEASE` is used to increment **push_cursor** or **pop_cursor**
- `__ATOMIC_ACQUIRE`: This ensures that the write to the buffer (storing the value) is completed and visible to the consumer before the updated push_cursor is published.
- `__ATOMIC_RELEASE`: This ensures that the write to the buffer (storing the value) is completed and visible to the consumer before the updated push_cursor is published.

Together, acquire-release pairing establishes a happens-before relationship

<!-- <div align="center">
<img src="img/sec.jpg" alt="ring" width="350">
</div> -->

### Code
```c
int ering_push(Ering *const ring, void *const value) {
    unsigned int pop_c;
    __atomic_load(&ring->pop_cursor, &pop_c,  __ATOMIC_ACQUIRE);

    if ((ring->push_cursor - pop_c) == ring->capacity)
        return 0;
    ring->ring[ring->push_cursor % ring->capacity] = value;
    __atomic_fetch_add(&ring->push_cursor, 1, __ATOMIC_RELEASE);
    return 1;
}

int ering_pop(Ering *const ring, void **const value) {
    unsigned int push_c;
    __atomic_load(&ring->push_cursor, &push_c,  __ATOMIC_ACQUIRE);
    if ((push_c - ring->pop_cursor) == 0)
        return 0;
    *value = ring->ring[ring->pop_cursor % ring->capacity];
    __atomic_fetch_add(&ring->pop_cursor, 1, __ATOMIC_RELEASE);
    return 1;
}
```

## Step 3: Fixing False Sharing
In this step, I modify the `Ering` structure to eliminate **false sharing**.

`False sharing` occurs when multiple threads access different variables that reside on the same CPU cache line (typically 64 bytes on modern systems).

Even if the variables are logically independent (e.g., one thread writes to **push_cursor** and another reads **pop_cursor**), the CPU cache coherence protocol invalidates and reloads the entire cache line for each thread’s operation. This causes unnecessary contention, slowing down performance as threads ping-pong the cache line between CPU cores.

In the previous step, *push_cursor* and *pop_cursor* were adjacent in memory within the Ering struct. Since they’re smaller than a cache line, they often share the same 64-byte cache line. When the producer updates push_cursor and the consumer reads pop_cursor, the cache line bounces between cores, degrading throughput.

### Code
```c
#define CACHE_LINE_SIZE 64

typedef struct {
    unsigned int capacity;
    void** ring;
    __attribute__((aligned(CACHE_LINE_SIZE))) unsigned int push_cursor;
    __attribute__((aligned(CACHE_LINE_SIZE))) unsigned int pop_cursor;
} __attribute__((aligned(CACHE_LINE_SIZE))) Ering;
```

## Step 4: Optimizing Cursor Reads with Caching
In this step, I optimized the ring by reducing unnecessary atomic reads of the opposing thread’s cursor.

In the previous step, the producer always read pop_cursor (via __atomic_load) on every ering_push call to check if the queue was full, and the consumer always read push_cursor on every ering_pop call to check if it was empty. However:

- The **producer** only needs the real `pop_cursor` value when the queue appears full (i.e., push_cursor - pop_cursor == capacity).

- The **consumer** only needs the real `push_cursor` value when the queue appears empty (i.e., push_cursor - pop_cursor == 0).

For most operations, the queue is neither full nor empty, so these atomic reads are unnecessary overhead.


### Code
```c
typedef struct {
    __attribute__((aligned(CACHE_LINE_SIZE))) unsigned int cached_push_cursor;
    __attribute__((aligned(CACHE_LINE_SIZE))) unsigned int cached_pop_cursor;
    __attribute__((aligned(CACHE_LINE_SIZE))) unsigned int push_cursor;
    __attribute__((aligned(CACHE_LINE_SIZE))) unsigned int pop_cursor;

    unsigned int capacity;
    void** ring;
} __attribute__((aligned(CACHE_LINE_SIZE))) Ering;

int ering_push(Ering *const ring, void *const value) {
    if (ring->push_cursor - ring->cached_pop_cursor == ring->capacity) {
        __atomic_load(&ring->pop_cursor, &ring->cached_pop_cursor,  __ATOMIC_ACQUIRE);
        if (ring->push_cursor - ring->cached_pop_cursor == ring->capacity) {
            return 0;
        }
    }
    ring->ring[ring->push_cursor % ring->capacity] = value;
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
    *value = ring->ring[ring->pop_cursor % ring->capacity];
    __atomic_fetch_add(&ring->pop_cursor, 1, __ATOMIC_RELEASE);
    return 1;
}
```

## Step 5: Replacing Modulo with Bitwise AND

In this step, I optimize the circular buffer indexing by replacing the modulo operator (`%`) with a bitwise AND (`&`). This change requires the ring’s capacity to be a power of 2, which we enforce during initialization.

- `align32pow2` Function: This utility rounds up the requested capacity to the next power of 2 (e.g., 5 becomes 8, 9 becomes 16).

- Modulo vs. AND: The modulo operation (%) is a division instruction, which is computationally expensive (often 10-20 CPU cycles or more, depending on the architecture). In contrast, a bitwise AND (&) is a single-cycle operation on most CPUs, making it significantly faster



### Code
```c
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

int ering_init(Ering *const ring, unsigned int capacity) {
    ...

    ring->capacity = align32pow2(capacity);
    ring->mask = ring->capacity - 1;

    ...
}

int ering_push(Ering *const ring, void *const value) {
    ...
    ring->ring[ring->push_cursor & ring->mask] = value;
    ...
}

int ering_pop(Ering *const ring, void **const value) {
    ...
    *value = ring->ring[ring->pop_cursor & ring->mask];
    ...
}

```

## Benchmark
To ensure more accurate results, I prepared [my system](#system-configuration) following the instructions outlined in [here](#system-prepration) before running the benchmarks.

Even though the thread sanitizer flags issues with step1 unsafe ring (as it’s not yet thread-safe), I have run benchmarks to establish a performance baseline.

Below are the results:


## System Preparation

To ensure reliable and fair evaluations, the first step is setting up a stable and dedicated environment. This means minimizing factors that could influence performance results in any benchmarking process. The provided [guide](https://easyperf.net/blog/2019/08/02/Perf-measurement-environment-on-Linux) explains how to set up a consistent Linux environment effectively.

**The following commands are executed:**

```bash
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```
- **What it does**: Sets the CPU frequency scaling governor to "performance" mode for all CPU cores. This locks the CPU frequency to its maximum value (1.90GHz for the Intel i5-4300U) rather than allowing dynamic scaling (e.g., powersave or ondemand modes).

- **Why it works**: Dynamic frequency scaling can cause performance fluctuations during benchmarks due to the CPU adjusting its clock speed based on load. By forcing a constant maximum frequency, we eliminate this variability, ensuring consistent execution times for push and pop operations in the FIFO queue.

```bash
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
```
- **What it does**: Disables Intel Turbo Boost, which temporarily increases CPU frequency beyond the base clock (up to ~2.5GHz for the i5-4300U) under high load.

- **Why it works**: Turbo Boost introduces non-deterministic performance spikes, making benchmark results less predictable. Disabling it ensures the CPU runs at a stable 1.90GHz, providing a controlled environment where performance depends solely on the code and not transient boosts.

```bash
echo off | sudo tee /sys/devices/system/cpu/smt/control
```
- **What it does**: Disables Simultaneous Multithreading (SMT), also known as Hyper-Threading, on the system. The Intel i5-4300U has 2 threads per core, so this reduces the 4 logical CPUs to 2 physical cores.

- **Why it works**: Hyper-Threading can lead to resource contention and uneven workload distribution between threads. Disabling it ensures that the producer and consumer threads run on dedicated physical cores, reducing interference and providing more consistent latency and throughput measurements. This aligns with the SPSC design goal of isolating the two threads.

I have used `bind_cpu` function to pin each thread to specific core.
```c
int bind_cpu(int core) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(core, &mask);
    return sched_setaffinity(0, sizeof(mask), &mask);
}
```

## System Configuration

This section details the hardware configuration used for benchmarking and testing the Ering implementation to ensure consistent and accurate results.

- **Processor**: Intel(R) Core(TM) i5-4300U CPU @ 1.90GHz
- **CPU(s)**: 4
  - **On-line CPU(s) list**: 0-3
  - **Thread(s) per core**: 2
- **Caches (sum of all)**:
  - **L1d**: 64 KiB (2 instances)
  - **L1i**: 64 KiB (2 instances)
  - **L2**: 512 KiB (2 instances)
  - **L3**: 3 MiB (1 instance)
- 4GB RAM
- Run Ubuntu 24.04.2 LTS with kernel 6.8.0-55-generic
- gcc-13.3.0
- clang-18.1.3
