cmake_minimum_required(VERSION 3.16)
project(draggableicons LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/draganddrop/draggableicons")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(draggableicons
    dragwidget.cpp dragwidget.h
    main.cpp
)

set_target_properties(draggableicons PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(draggableicons PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

# Resources:
set(draggableicons_resource_files
    "images/boat.png"
    "images/car.png"
    "images/house.png"
)

qt_add_resources(draggableicons "draggableicons"
    PREFIX
        "/"
    FILES
        ${draggableicons_resource_files}
)

install(TARGETS draggableicons
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
