
cmake_minimum_required(VERSION 2.8)

project(test_wrap_cpp_options)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
endif()

find_package(Qt5Core REQUIRED HINTS ${Qt5Tests_PREFIX_PATH})

include_directories(${Qt5Core_INCLUDE_DIRS})

add_definitions(${Qt5Core_DEFINITIONS})

# Test options. The -i option removes the include "myobject.h" from the moc file
# causing a compile failure. -> Options work
qt5_wrap_cpp(moc_files myobject.h OPTIONS -i)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

add_executable(myobject myobject.cpp ${moc_files})
target_link_libraries(myobject ${Qt5Core_LIBRARIES})
