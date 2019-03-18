# Generated from tablemodel.pro.

#####################################################################
## tablemodel Binary:
#####################################################################

add_qt_executable(tablemodel
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/tablemodel"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/tablemodel"
    SOURCES
        ../connection.h
        tablemodel.cpp
    LIBRARIES
        Qt::Sql
        Qt::Widgets
)
