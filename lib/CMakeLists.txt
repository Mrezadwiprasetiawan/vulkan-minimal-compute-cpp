cmake_minimum_required(VERSION 3.31.2)

project(vkmincomp)

find_package(Vulkan)


#uncomment salah satu
add_library(${PROJECT_NAME} SHARED src/stdEng.cxx)
#add_library(${PROJECT_NAME} STATIC src/stdEng.cxx)

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
