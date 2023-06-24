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

# Config
add_library(config INTERFACE)
target_include_directories(config INTERFACE config/include)

# Fugax
set(fugax_source_files
    fugax/src/event.cpp
    fugax/src/event-loop.cpp
    fugax/src/event-guard.cpp
)
add_library(fugax ${fugax_source_files})
target_include_directories(fugax PUBLIC fugax/include)
target_link_libraries(fugax PUBLIC config juro iara-utils)
configure_file(
    ${PROJECT_SOURCE_DIR}/config/include/config/fugax.hpp.in
    ${PROJECT_SOURCE_DIR}/config/include/config/fugax.hpp
    @ONLY
)

# Fuss
add_library(fuss INTERFACE)
target_include_directories(fuss INTERFACE fuss/include)

# Juro
set(juro_source_files juro/src/promise.cpp juro/src/compose/all.cpp)
add_library(juro ${juro_source_files})
target_include_directories(juro PUBLIC juro/include utils/include)

# Plumbing
add_library(plumbing INTERFACE)
target_link_libraries(plumbing INTERFACE iara-utils fuss)
target_include_directories(plumbing INTERFACE plumbing/include)

# Iara
add_library(iara ${juro_source_files} ${fugax_source_files})
target_include_directories(iara PUBLIC fuss/include fugax/include juro/include plumbing/include utils/include)

# Utils
add_library(iara-utils INTERFACE)
target_include_directories(iara-utils INTERFACE utils/include)

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

