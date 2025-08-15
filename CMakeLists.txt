cmake_minimum_required(VERSION 3.10)
project(KVStorage
        VERSION 1.0.0
        DESCRIPTION ""
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.15.3
)
FetchContent_MakeAvailable(spdlog)

add_library(KVStorage SHARED
        src/dict/Dictionary.h
        src/dict/Bucket.h
        src/InMemoryKVStorage.h
        include/KVStorage.h
)

target_link_libraries(KVStorage PUBLIC
        spdlog
)

target_include_directories(KVStorage PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)


enable_testing()
add_subdirectory(tests)