
cmake_minimum_required(VERSION 2.8)

project(test_modules)

set(qtbase_modules
    Core
    Concurrent
    Gui
    Widgets
    Network
    OpenGL
    Sql
    Test
    Xml
    PrintSupport
)

if (UNIX AND NOT APPLE AND NOT QNXNTO)
    add_definitions(-DEXPECT_DBUS_AVAILABLE)
    list(APPEND qtbase_modules DBus)
endif()

foreach(_module ${qtbase_modules})
    find_package(Qt5${_module} REQUIRED)

    include_directories(
        ${Qt5${_module}_INCLUDE_DIRS}
    )

    add_definitions(
        ${Qt5${_module}_DEFINITIONS}
    )
endforeach()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

add_executable(mainapp main.cpp)

foreach(_module ${qtbase_modules})
    target_link_libraries(mainapp
        ${Qt5${_module}_LIBRARIES}
    )
endforeach()
