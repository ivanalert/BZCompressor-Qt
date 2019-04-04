cmake_minimum_required(VERSION 2.8.9)

project(BZCompressor)

if (CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-std=c++11)
endif()

find_package(Qt5 REQUIRED Core)

set(BINARY_DIR "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BINARY_DIR}/lib")

add_subdirectory(compressor)
add_subdirectory(bzcompressor)
