cmake_minimum_required(VERSION 3.16)
project(collidingmice LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/collidingmice")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(collidingmice
    main.cpp
    mouse.cpp mouse.h
)

set_target_properties(collidingmice PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(collidingmice PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

# Resources:
set(mice_resource_files
    "images/cheese.jpg"
)

qt_add_resources(collidingmice "mice"
    PREFIX
        "/"
    FILES
        ${mice_resource_files}
)

install(TARGETS collidingmice
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
