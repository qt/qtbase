# Generated from frozencolumn.pro.

cmake_minimum_required(VERSION 3.14)
project(frozencolumn LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/itemviews/frozencolumn")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(frozencolumn
    freezetablewidget.cpp freezetablewidget.h
    main.cpp
)
set_target_properties(frozencolumn PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(frozencolumn PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(grades_resource_files
    "grades.txt"
)

qt6_add_resources(frozencolumn "grades"
    PREFIX
        "/"
    FILES
        ${grades_resource_files}
)

install(TARGETS frozencolumn
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
