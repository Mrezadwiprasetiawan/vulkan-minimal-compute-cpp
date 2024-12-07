cmake_minimum_required(VERSION 3.31.1)
project(run)

# Menemukan Vulkan
find_package(Vulkan REQUIRED)

# Kompilasi shader HLSL ke SPV menggunakan glslc
add_custom_command(
  OUTPUT compute.spv  # File output yang dihasilkan
  COMMAND glslc -fshader-stage=compute "compute.hlsl" -o "compute.spv"
  DEPENDS "compute.hlsl"  # Menandakan bahwa file .hlsl dibutuhkan sebelum menjalankan perintah ini
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}  # Jalankan dari direktori sumber
  COMMENT "Mengkompilasi Shader"  # Komentar yang ditampilkan saat proses build
)

# Menambahkan target custom untuk shader
add_custom_target(compShader DEPENDS compute.spv)

# Menambahkan eksekutabel utama
add_executable(${PROJECT_NAME} "main.cpp")

# Menghubungkan eksekutabel dengan Vulkan
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)

# Menambahkan ketergantungan pada target compShader
add_dependencies(${PROJECT_NAME} compShader)
