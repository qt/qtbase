# Generated from puzzle.pro.

cmake_minimum_required(VERSION 3.14)
project(puzzle LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(itemviews_puzzle # special case: renamed puzzle
    main.cpp
    mainwindow.cpp mainwindow.h
    piecesmodel.cpp piecesmodel.h
    puzzle.qrc
    puzzlewidget.cpp puzzlewidget.h
)
target_link_libraries(itemviews_puzzle PUBLIC # special case: renamed puzzle
    Qt::Widgets
)

install(TARGETS itemviews_puzzle # special case: renamed puzzle
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
