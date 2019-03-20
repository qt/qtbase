# Generated from calendarwidget.pro.

#####################################################################
## calendarwidget Binary:
#####################################################################

add_qt_executable(calendarwidget
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/calendarwidget"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/calendarwidget"
    SOURCES
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)
