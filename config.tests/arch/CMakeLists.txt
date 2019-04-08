cmake_minimum_required(VERSION 3.15.0)
project(arch LANGUAGES CXX)

add_executable(architecture_test)
set_property(TARGET architecture_test PROPERTY MACOSX_BUNDLE FALSE)
target_sources(architecture_test PRIVATE arch.cpp)
