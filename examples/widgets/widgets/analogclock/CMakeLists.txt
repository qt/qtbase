# Generated from analogclock.pro.

#####################################################################
## analogclock Binary:
#####################################################################

add_qt_executable(analogclock_widgets # special case
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/analogclock"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/widgets/analogclock"
    SOURCES
        analogclock.cpp analogclock.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:analogclock.pro:<NONE>:
# QMAKE_PROJECT_NAME = "widgets_analogclock"
