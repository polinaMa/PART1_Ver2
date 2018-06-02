cmake_minimum_required(VERSION 3.6)
project(DeduplicationProject)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c List.c Block.c Directory.c File.c HashTable.c HashTableF.c Utilities.c TextParsing.c memory_pool.c)
add_executable(DeduplicationProject ${SOURCE_FILES})