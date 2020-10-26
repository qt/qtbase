# Generated from puzzle.pro.

cmake_minimum_required(VERSION 3.14)
project(puzzle LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/itemviews/puzzle")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(itemviews_puzzle # special case: renamed puzzle
    main.cpp
    mainwindow.cpp mainwindow.h
    piecesmodel.cpp piecesmodel.h
    puzzlewidget.cpp puzzlewidget.h
)
set_target_properties(itemviews_puzzle PROPERTIES # special case
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(itemviews_puzzle PUBLIC # special case
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(puzzle_resource_files
    "example.jpg"
)

qt6_add_resources(itemviews_puzzle "puzzle"
    PREFIX
        "/images"
    FILES
        ${puzzle_resource_files}
)

install(TARGETS itemviews_puzzle # special case: renamed puzzle
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
