
cmake_minimum_required(VERSION 2.8)

project(pass4)

find_package(Qt5Core REQUIRED)

include_directories(${Qt5Core_INCLUDE_DIRS})

add_definitions(${Qt5Core_DEFINITIONS})

qt5_wrap_cpp(moc_files myobject.h)

# Test options. The -binary option generates a binary to dlopen instead of
# a source file to compile. The compiler will consider it garbage when used
# in the add_executable call.
qt5_add_resources(rcc_files "pass4.qrc" OPTIONS -binary)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

add_executable(myobject myobject.cpp ${moc_files} ${rcc_files})
target_link_libraries(myobject ${Qt5Core_LIBRARIES})
