cmake_minimum_required(VERSION 3.31.2)

project(vkmincomp VERSION 0.2 LANGUAGES CXX)

find_package(Vulkan  REQUIRED)

include_directories(include)

add_subdirectory(quick)
add_subdirectory(lib)
