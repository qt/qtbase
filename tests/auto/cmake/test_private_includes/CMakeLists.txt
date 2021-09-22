
cmake_minimum_required(VERSION 3.16)

project(test_private_includes)

find_package(Qt6Gui REQUIRED)

add_executable(testapp main.cpp)
target_link_libraries(testapp PRIVATE Qt::GuiPrivate)
