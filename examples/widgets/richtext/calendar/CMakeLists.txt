# Generated from calendar.pro.

#####################################################################
## calendar Binary:
#####################################################################

add_qt_executable(calendar
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/richtext/calendar"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/richtext/calendar"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
    LIBRARIES
        Qt::Widgets
)
