
cmake_minimum_required(VERSION 2.8)

project(test_add_resource_options)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
endif()

find_package(Qt5Core REQUIRED HINTS ${Qt5Tests_PREFIX_PATH})

include_directories(${Qt5Core_INCLUDE_DIRS})

add_definitions(${Qt5Core_DEFINITIONS})

qt5_wrap_cpp(moc_files myobject.h)

# Test options. The -binary option generates a binary to dlopen instead of
# a source file to compile. The compiler will consider it garbage when used
# in the add_executable call.
qt5_add_resources(rcc_files "test_macro_options.qrc" OPTIONS -binary)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

add_executable(myobject myobject.cpp ${moc_files} ${rcc_files})
target_link_libraries(myobject ${Qt5Core_LIBRARIES})
