# Generated from codeeditor.pro.

#####################################################################
## codeeditor Binary:
#####################################################################

add_qt_executable(codeeditor
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/codeeditor"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/codeeditor"
    SOURCES
        codeeditor.cpp codeeditor.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)
