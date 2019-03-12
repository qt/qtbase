cmake_minimum_required(VERSION 3.14.0)
project(x86_simd LANGUAGES CXX)

include(../../cmake/QtPlatformSupport.cmake)
include(../../cmake/QtCompilerOptimization.cmake)

string(TOUPPER "${SIMD}" upper_simd)

if(NOT DEFINED "QT_CFLAGS_${upper_simd}")
    message(FATAL_ERROR "This compiler does not support ${SIMD}.")
endif()

add_executable("SimdTest${SIMD}")
target_sources("SimdTest${SIMD}" PRIVATE main.cpp)
target_compile_options("SimdTest${SIMD}" PRIVATE ${QT_CFLAGS_${upper_simd}})
