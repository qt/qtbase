# Generated from analogclock.pro.

#####################################################################
## analogclock Binary:
#####################################################################

add_qt_executable(analogclock
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/gui/analogclock"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/gui/analogclock"
    SOURCES
        ../rasterwindow/rasterwindow.cpp ../rasterwindow/rasterwindow.h
        main.cpp
    INCLUDE_DIRECTORIES
        ../rasterwindow
    LIBRARIES # special case
        Qt::Gui
)

#### Keys ignored in scope 1:.:analogclock.pro:<NONE>:
# CONFIG = "no_batch"
