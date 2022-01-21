cmake_minimum_required(VERSION 3.16)
project(dragdroprobot LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/dragdroprobot")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(dragdroprobot
    coloritem.cpp coloritem.h
    main.cpp
    robot.cpp robot.h
)

set_target_properties(dragdroprobot PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(dragdroprobot PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

# Resources:
set(robot_resource_files
    "images/head.png"
)

qt_add_resources(dragdroprobot "robot"
    PREFIX
        "/"
    FILES
        ${robot_resource_files}
)

install(TARGETS dragdroprobot
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
