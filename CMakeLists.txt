cmake_minimum_required(VERSION 3.14)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#set(CMAKE_C_DEPFILE_FORMAT msvc)

#-DCMAKE_CXX_COMPILER=dpcpp -DOpenMP_CXX_FLAGS="-qopenmp" -DOpenMP_CXX_LIB_NAMES="libiomp5" -DOpenMP_libiomp5_LIBRARY=/opt/intel/inteloneapi/compiler/2021.1-beta03/linux/compiler/lib/intel64_lin/libiomp5.so

set(CMAKE_CXX_COMPILER "icpx")
set(CMAKE_C_COMPILER "icx")

#set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
#set(CMAKE_C_COMPILER_WORKS TRUE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_BUILD_TYPE Debug)

find_package(SDL2 REQUIRED)

#add_compile_options(-Og -qopt-report=max -debug)
#add_compile_options(-O3 -qopenmp -shared-intel -xCORE-AVX2 -qopt-report=max -debug)
#add_compile_options(-Og -shared-intel -qopt-report=max -debug)

#/arch:CORE-AVX2 -Qopenmp /QxCORE-AVX2 -qopenmp 



include_directories(imgui/)
include_directories(imgui/backends/)

file(GLOB IMGUI1 imgui/*.cpp)
file(GLOB IMGUI2 imgui/*.h)
# file(GLOB IMGUI3 imgui/backends/*.cpp)
# file(GLOB IMGUI4 imgui/backends/*.h)
add_library(imgui SHARED
  ${IMGUI1}
  ${IMGUI2}
  imgui/backends/imgui_impl_sdl2.cpp
  imgui/backends/imgui_impl_sdl2.h
  imgui/backends/imgui_impl_opengl2.cpp
  imgui/backends/imgui_impl_opengl2.cpp
  imgui/backends/imgui_impl_sdlrenderer2.h
  imgui/backends/imgui_impl_sdlrenderer2.h
  )

target_compile_options(imgui PRIVATE -O3 -xCORE-AVX2)

include_directories(/usr/include/SDL2)
include_directories(/usr/include/GL)
include_directories(/usr/include/glut)

#include_directories(~/intel/oneapi/2024.1/include)

project(homework)
add_executable(homework main.cpp)
target_sources(homework PUBLIC nbody.hpp nbody.cpp args.hxx)
#add_executable(homework PUBLIC main.cpp)

#target_compile_options(homework PRIVATE -Og -qopt-report=max -debug)
#target_compile_options(homework PRIVATE -Og -debug )
#target_compile_options(homework PRIVATE -O3 -debug -xCORE-AVX2 -qopenmp-stubs)
target_compile_options(homework PRIVATE -O3 -debug -xCORE-AVX2 -qopenmp)

#find_library(NAMES omp PATHS ~/intel/oneapi/2024.1/lib)

target_link_libraries(homework imgui)
target_link_libraries(homework GL)
target_link_libraries(homework GLU)
target_link_libraries(homework SDL2)
target_link_libraries(homework omp)
#target_link_libraries(homework ~/intel/oneapi/2024.1/lib/libiomp5.so)
message(STATUS ${CMAKE_CURRENT_SOURCE_DIR})
