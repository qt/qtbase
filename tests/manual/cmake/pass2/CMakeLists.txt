
cmake_minimum_required(VERSION 2.8)

project(pass2)

find_package(Qt5Core REQUIRED)

include_directories(${Qt5Core_INCLUDE_DIRS})

qt5_wrap_cpp(moc_files myobject.h)

qt5_add_resources(rcc_files "pass2.qrc")

add_executable(myobject myobject.cpp ${moc_files} ${rcc_files})
target_link_libraries(myobject ${Qt5Core_LIBRARIES})
