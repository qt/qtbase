# Generated from sqlwidgetmapper.pro.

#####################################################################
## sqlwidgetmapper Binary:
#####################################################################

add_qt_executable(sqlwidgetmapper
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/sqlwidgetmapper"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/sqlwidgetmapper"
    SOURCES
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Sql
        Qt::Widgets
)
