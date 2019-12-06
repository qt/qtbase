
cmake_minimum_required(VERSION 3.9)

project(test_add_big_resource)

find_package(Qt5Core REQUIRED)

qt5_wrap_cpp(moc_files myobject.h)

qt5_add_big_resources(rcc_files "test_add_big_resource.qrc" "test_add_big_resource2.qrc")

add_executable(myobject myobject.cpp ${moc_files} ${rcc_files})
target_link_libraries(myobject ${Qt5Core_LIBRARIES})
