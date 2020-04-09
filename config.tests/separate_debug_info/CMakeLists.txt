# special case skip regeneration
cmake_minimum_required(VERSION 3.14.0)
project(objcopytest LANGUAGES CXX)
include(CMakeFindBinUtils)
add_executable(objcopytest main.cpp)
add_custom_command(
    TARGET objcopytest
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:objcopytest> objcopytest.debug
    COMMAND ${CMAKE_OBJCOPY} --strip-debug $<TARGET_FILE:objcopytest>
    COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=objcopytest.debug $<TARGET_FILE:objcopytest>)
