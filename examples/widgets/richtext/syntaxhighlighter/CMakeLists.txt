# Generated from syntaxhighlighter.pro.

#####################################################################
## syntaxhighlighter Binary:
#####################################################################

add_qt_executable(syntaxhighlighter
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/richtext/syntaxhighlighter"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/richtext/syntaxhighlighter"
    SOURCES
        highlighter.cpp highlighter.h
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Widgets
)
