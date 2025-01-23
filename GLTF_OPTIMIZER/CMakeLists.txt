# 设置最低版本要求
cmake_minimum_required(VERSION 3.22)

# 设置项目名称
project(GLTF_OPTIMIZER)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/arm64-osx")
find_package(boost_filesystem REQUIRED CONFIG)
MESSAGE(STATUS "boost_filesystem: ${boost_filesystem_FOUND} ${boost_INCLUDE_DIR}")
find_package(fmt CONFIG REQUIRED)

set(GLTF_OPTIMIZER_SOURCES
        globals.h
        gltfExporter.cpp
        gltfExporter.h
        LodExporter.cpp
        LodExporter.h
        main.cpp
        MeshOptimizer.cpp
        MeshOptimizer.h
        MyMesh.h
        Pipeline.cpp
        Pipeline.h
        SpatialTree.cpp
        SpatialTree.h
        utils.cpp
        utils.h)
#find_package(vcg REQUIRED)
add_executable(main ${GLTF_OPTIMIZER_SOURCES})
set_target_properties(main PROPERTIES OUTPUT_NAME "GLTF_OPTIMIZER")
target_include_directories(main PUBLIC ./tinyGltf ${VCG_LIB} "${VCG_LIB}/eigenlib")
target_link_libraries(main PUBLIC fmt::fmt-header-only)
target_link_libraries(main PUBLIC Boost::filesystem)