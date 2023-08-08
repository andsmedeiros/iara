cmake_minimum_required(VERSION 3.5)
project(iara LANGUAGES CXX VERSION 0.0.1)

Include(FetchContent)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_VERBOSE_MAKEFILE ON)

if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

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
target_include_directories(iara PUBLIC config/include fuss/include fugax/include juro/include plumbing/include utils/include)

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

# Fugax tests
set(fugax_test_source_files test/src/fugax/test.cpp)

# Fuss tests
set(fuss_test_source_files test/src/fuss/test.cpp)

# Juro tests
set(juro_test_source_files test/src/juro/test.cpp)

add_executable(iara-test
        ${fugax_test_source_files}
        ${fuss_test_source_files}
        ${juro_test_source_files}
)
target_link_libraries(iara-test PRIVATE juro fuss fugax Catch2::Catch2WithMain)
target_include_directories(iara-test PUBLIC test/include)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(iara-test)