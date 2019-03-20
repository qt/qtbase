# Generated from charactermap.pro.

#####################################################################
## charactermap Binary:
#####################################################################

add_qt_executable(charactermap
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/charactermap"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/charactermap"
    SOURCES
        characterwidget.cpp characterwidget.h
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Widgets
)
