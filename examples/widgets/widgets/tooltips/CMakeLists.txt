cmake_minimum_required(VERSION 3.16)
project(tooltips LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/widgets/tooltips")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(tooltips
    main.cpp
    shapeitem.cpp shapeitem.h
    sortingbox.cpp sortingbox.h
)

set_target_properties(tooltips PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(tooltips PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

# Resources:
set(tooltips_resource_files
    "images/circle.png"
    "images/square.png"
    "images/triangle.png"
)

qt_add_resources(tooltips "tooltips"
    PREFIX
        "/"
    FILES
        ${tooltips_resource_files}
)

install(TARGETS tooltips
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
