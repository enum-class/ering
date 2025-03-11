cmake_minimum_required(VERSION 3.16)

project(ERing C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED On)

add_compile_options(-Wall -Wextra -pedantic -Werror)

# unsafe_ring
add_executable(unsafe_unittests unit_tests.c)
target_compile_definitions(unsafe_unittests PRIVATE UNSAFE_RING)
target_compile_options(unsafe_unittests PRIVATE -fsanitize=address -g -O0)
target_link_options(unsafe_unittests PRIVATE -fsanitize=address)

add_executable(unsafe_bench_tsan bench.c)
target_compile_definitions(unsafe_bench_tsan PRIVATE UNSAFE_RING)
target_compile_options(unsafe_bench_tsan PRIVATE -fsanitize=thread -g -O0)
target_link_options(unsafe_bench_tsan PRIVATE -fsanitize=thread)

add_executable(unsafe_bench bench.c)
target_compile_definitions(unsafe_bench PRIVATE UNSAFE_RING)
target_compile_definitions(unsafe_bench PRIVATE NDEBUG)
target_compile_options(unsafe_bench PRIVATE -O2)

# safe_ring
add_executable(safe_unittests unit_tests.c)
target_compile_definitions(safe_unittests PRIVATE SAFE_RING)
target_compile_options(safe_unittests PRIVATE -fsanitize=address -g -O0)
target_link_options(safe_unittests PRIVATE -fsanitize=address)

add_executable(safe_bench_tsan bench.c)
target_compile_definitions(safe_bench_tsan PRIVATE SAFE_RING)
target_compile_options(safe_bench_tsan PRIVATE -fsanitize=thread -g -O0)
target_link_options(safe_bench_tsan PRIVATE -fsanitize=thread)

add_executable(safe_bench bench.c)
target_compile_definitions(safe_bench PRIVATE SAFE_RING)
target_compile_definitions(safe_bench PRIVATE NDEBUG)
target_compile_options(safe_bench PRIVATE -O2)

# false_sharing_ring
add_executable(false_unittests unit_tests.c)
target_compile_definitions(false_unittests PRIVATE FALSE_SHARING_RING)
target_compile_options(false_unittests PRIVATE -fsanitize=address -g -O0)
target_link_options(false_unittests PRIVATE -fsanitize=address)

add_executable(false_bench_tsan bench.c)
target_compile_definitions(false_bench_tsan PRIVATE FALSE_SHARING_RING)
target_compile_options(false_bench_tsan PRIVATE -fsanitize=thread -g -O0)
target_link_options(false_bench_tsan PRIVATE -fsanitize=thread)

add_executable(false_bench bench.c)
target_compile_definitions(false_bench PRIVATE FALSE_SHARING_RING)
target_compile_definitions(false_bench PRIVATE NDEBUG)
target_compile_options(false_bench PRIVATE -O2)

# cache_ring
add_executable(cache_unittests unit_tests.c)
target_compile_definitions(cache_unittests PRIVATE CACHE_RING)
target_compile_options(cache_unittests PRIVATE -fsanitize=address -g -O0)
target_link_options(cache_unittests PRIVATE -fsanitize=address)

add_executable(cache_bench_tsan bench.c)
target_compile_definitions(cache_bench_tsan PRIVATE CACHE_RING)
target_compile_options(cache_bench_tsan PRIVATE -fsanitize=thread -g -O0)
target_link_options(cache_bench_tsan PRIVATE -fsanitize=thread)

add_executable(cache_bench bench.c)
target_compile_definitions(cache_bench PRIVATE CACHE_RING)
target_compile_definitions(cache_bench PRIVATE NDEBUG)
target_compile_options(cache_bench PRIVATE -O2)

# mod_ring
add_executable(mod_unittests unit_tests.c)
target_compile_definitions(mod_unittests PRIVATE MOD_RING)
target_compile_options(mod_unittests PRIVATE -fsanitize=address -g -O0)
target_link_options(mod_unittests PRIVATE -fsanitize=address)

add_executable(mod_bench_tsan bench.c)
target_compile_definitions(mod_bench_tsan PRIVATE MOD_RING)
target_compile_options(mod_bench_tsan PRIVATE -fsanitize=thread -g -O0)
target_link_options(mod_bench_tsan PRIVATE -fsanitize=thread)

add_executable(mod_bench bench.c)
target_compile_definitions(mod_bench PRIVATE MOD_RING)
target_compile_definitions(mod_bench PRIVATE NDEBUG)
target_compile_options(mod_bench PRIVATE -O2)
