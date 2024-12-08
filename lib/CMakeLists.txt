cmake_minimum_required(VERSION 3.31.2)

project(vkmincomp)

find_package(Vulkan)

add_library(${PROJECT_NAME} SHARED src/vkmincomp.cxx)

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
