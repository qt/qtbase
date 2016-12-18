
cmake_minimum_required(VERSION 3.0)

project(test_private_targets)

find_package(Qt5Gui REQUIRED)

add_executable(testapp main.cpp)
target_link_libraries(testapp Qt5::GuiPrivate)
