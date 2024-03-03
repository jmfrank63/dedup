cmake_minimum_required(VERSION 3.10)

# Set the project name
project (dedup)

# Add the directory with the blake3.h file to the include paths
include_directories(submodules/BLAKE3/c)

# Add an executable
add_executable(dedup dedup.c)
