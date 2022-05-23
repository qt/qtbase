cmake_minimum_required(VERSION 3.16)
project(direct_extern_access LANGUAGES CXX)

# this is the test found in https://sourceware.org/bugzilla/show_bug.cgi?id=29087

add_library(no_extern_access_lib SHARED lib.cpp)
add_executable(no_extern_access_main main.cpp)
target_compile_options(no_extern_access_lib PUBLIC "-mno-direct-extern-access")
target_link_libraries(no_extern_access_main no_extern_access_lib)
