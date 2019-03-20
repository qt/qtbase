# Generated from puzzle.pro.

#####################################################################
## puzzle Binary:
#####################################################################

add_qt_executable(puzzle
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/draganddrop/puzzle"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/draganddrop/puzzle"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
        pieceslist.cpp pieceslist.h
        puzzlewidget.cpp puzzlewidget.h
    LIBRARIES
        Qt::Widgets
)

# Resources:
add_qt_resource(puzzle "puzzle" PREFIX "/images" FILES
    example.jpg)


#### Keys ignored in scope 1:.:puzzle.pro:<NONE>:
# QMAKE_PROJECT_NAME = "dndpuzzle"
