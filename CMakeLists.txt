# 设置最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(GLTF_TO_3DTiles)

set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

# 添加子项目
add_subdirectory(GLTF_OPTIMIZER)