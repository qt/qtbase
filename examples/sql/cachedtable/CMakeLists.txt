# Generated from cachedtable.pro.

#####################################################################
## cachedtable Binary:
#####################################################################

add_qt_executable(cachedtable
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/cachedtable"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/cachedtable"
    SOURCES
        ../connection.h
        main.cpp
        tableeditor.cpp tableeditor.h
    LIBRARIES
        Qt::Sql
        Qt::Widgets
)
