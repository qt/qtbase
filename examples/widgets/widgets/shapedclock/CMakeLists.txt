# Generated from shapedclock.pro.

#####################################################################
## shapedclock Binary:
#####################################################################

add_qt_executable(shapedclock
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/shapedclock"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/shapedclock"
    SOURCES
        main.cpp
        shapedclock.cpp shapedclock.h
    LIBRARIES
        Qt::Widgets
)
