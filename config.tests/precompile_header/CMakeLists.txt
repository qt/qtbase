cmake_minimum_required(VERSION 3.15.0)
project(precompile_header LANGUAGES CXX)

add_executable(precompile_header main.cpp)
target_precompile_headers(precompile_header PRIVATE header.h)
