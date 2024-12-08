cmake_minimum_required(VERSION 3.31.2)

project(MyProject VERSION 0.2 LANGUAGES CXX)

find_package(Vulkan  REQUIRED)

#untuk exe
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#untuk shared
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#untuk static
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
include_directories(include)

add_subdirectory(quick)
add_subdirectory(lib)
