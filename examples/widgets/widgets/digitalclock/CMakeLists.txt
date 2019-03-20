# Generated from digitalclock.pro.

#####################################################################
## digitalclock Binary:
#####################################################################

add_qt_executable(digitalclock
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/digitalclock"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/digitalclock"
    SOURCES
        digitalclock.cpp digitalclock.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)
