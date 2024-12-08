set(EXE_SRC main.cxx)
set(EXE_NAME run)

add_executable(${EXE_NAME} ${EXE_SRC})

# Gunakan CMAKE_SOURCE_DIR untuk mendefinisikan path yang benar ke shaders
set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_HLSL "${SHADER_SOURCE_DIR}/compute.hlsl")
set(SHADER_SPV "${SHADER_SOURCE_DIR}/compute.spv")

# Definisikan custom command untuk mengkompilasi shader
add_custom_command(
    OUTPUT ${SHADER_SPV}
    COMMAND glslc -fshader-stage=compute ${SHADER_HLSL} -o ${SHADER_SPV}
    DEPENDS ${SHADER_HLSL}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Mengkompilasi Shader"
)

add_custom_target(compShader DEPENDS ${SHADER_SPV})

target_link_libraries(${EXE_NAME} PRIVATE Vulkan::Vulkan)

# Menambahkan dependensi antara executable dan target shader
add_dependencies(${EXE_NAME} compShader)
