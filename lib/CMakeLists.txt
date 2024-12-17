include_directories(${CMAKE_CURRENT_SOURCE_DIR} include)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SOURCE_FILES ${SOURCE_DIR}/stdEng.cxx)

#uncomment salah satu
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
#add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
