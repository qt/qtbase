
cmake_minimum_required(VERSION 2.8)

project(test_qtmainwin_library)

find_package(Qt5Core REQUIRED)

include_directories(${Qt5Core_INCLUDE_DIRS})

add_definitions(${Qt5Core_DEFINITIONS})

qt5_wrap_cpp(moc_files myobject.h)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

# On non-windows, the WIN32 is harmless, and Qt5Core_QTMAIN_LIBRARIES is empty.
# We test that it is harmless on those, and test that it builds on Windows.
# It wouldn't build if WIN32 is used and Qt5Core_QTMAIN_LIBRARIES is empty.
add_executable(myobject WIN32 myobject.cpp ${moc_files} )
target_link_libraries(myobject ${Qt5Core_LIBRARIES} ${Qt5Core_QTMAIN_LIBRARIES})
