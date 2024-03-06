cmake_minimum_required(VERSION 3.10)

# Set the project name
project (dedup LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include paths
include_directories(submodules/BLAKE3/c)
include_directories(submodules/criterion/include)

link_directories(submodules/criterion/build/src)

# Add an executable
add_executable(dedup src/dedup.c)
add_executable(test_ring_buffer tests/test_ring_buffer.c lib/ring_buffer.c)

target_link_libraries(test_ring_buffer criterion)
