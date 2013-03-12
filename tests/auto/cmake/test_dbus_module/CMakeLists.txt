
cmake_minimum_required(VERSION 2.8)

project(test_dbus_module)

find_package(Qt5DBus 5.0.0 REQUIRED)

include_directories(
    ${Qt5DBus_INCLUDE_DIRS}
)

add_definitions(${Qt5DBus_DEFINITIONS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5DBus_EXECUTABLE_COMPILE_FLAGS}")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(my_srcs mydbusobject.cpp)

qt5_wrap_cpp(moc_files mydbusobject.h)

qt5_generate_dbus_interface(
    mydbusobject.h
    ${CMAKE_BINARY_DIR}/org.qtProject.Tests.MyDBusObject.xml
)

qt5_add_dbus_adaptor(my_srcs
    ${CMAKE_BINARY_DIR}/org.qtProject.Tests.MyDBusObject.xml
    mydbusobject.h
    MyDBusObject
)

add_executable(myobject ${my_srcs} ${moc_files})
target_link_libraries(myobject ${Qt5DBus_LIBRARIES})
