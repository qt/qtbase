
cmake_minimum_required(VERSION 3.14)

project(test_add_big_resource)

find_package(Qt6Core REQUIRED)

qt_wrap_cpp(moc_files myobject.h)

qt_add_big_resources(rcc_files "test_add_big_resource.qrc" "test_add_big_resource2.qrc")

add_executable(myobject myobject.cpp ${moc_files} ${rcc_files})
target_link_libraries(myobject PRIVATE Qt::Core)
