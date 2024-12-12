cmake_minimum_required(VERSION 3.31.2)

project(vkmincomp)

find_package(Vulkan)


#uncomment salah satu
#add_library(${PROJECT_NAME} SHARED src/vkmincomp.cxx)
#add_library(${PROJECT_NAME} STATIC src/vkmincomp.cxx)

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
