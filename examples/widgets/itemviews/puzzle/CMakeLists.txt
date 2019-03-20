# Generated from puzzle.pro.

#####################################################################
## puzzle Binary:
#####################################################################

add_qt_executable(puzzle_itemview # special case
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/puzzle"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/itemviews/puzzle"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
        piecesmodel.cpp piecesmodel.h
        puzzlewidget.cpp puzzlewidget.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(puzzle_itemview "puzzle" PREFIX "/images" FILES # special case
    example.jpg)

