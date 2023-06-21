cmake_minimum_required(VERSION 3.5)
project(iara LANGUAGES CXX VERSION 0.0.1)

Include(FetchContent)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/dist/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/dist/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/dist/lib)

# Public libraries

# Juro
set(juro_source_files juro/src/promise.cpp juro/src/compose/all.cpp)
add_library(juro ${juro_source_files})
target_include_directories(juro PUBLIC juro/include utils/include)

# Loopr
set(loopr_source_files loopr/src/event.cpp loopr/src/event-loop.cpp)
add_library(loopr ${loopr_source_files})
target_include_directories(loopr PUBLIC loopr/include juro/include utils/include)
target_link_libraries(loopr INTERFACE juro)

# Iara
add_library(iara ${juro_source_files} ${loopr_source_files})
target_include_directories(iara PUBLIC fuss/include loopr/include juro/include plumbing/include utils/include)


# Tests

# Tests are written with Catch2
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.3.2
)
FetchContent_MakeAvailable(Catch2)
include(Catch)

# Juro tests
add_executable(test-juro juro/test/src/test.cpp)
target_link_libraries(test-juro PRIVATE juro Catch2::Catch2WithMain)
target_include_directories(test-juro PUBLIC juro/test/include)
set_property(TARGET test-juro PROPERTY ARCHIVE_OUTPUT_DIRECTORY dist/bin/test)
catch_discover_tests(test-juro)

