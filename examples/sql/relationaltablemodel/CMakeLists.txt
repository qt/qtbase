# Generated from relationaltablemodel.pro.

#####################################################################
## relationaltablemodel Binary:
#####################################################################

add_qt_executable(relationaltablemodel
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/relationaltablemodel"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/relationaltablemodel"
    SOURCES
        ../connection.h
        relationaltablemodel.cpp
    LIBRARIES
        Qt::Sql
        Qt::Widgets
)
