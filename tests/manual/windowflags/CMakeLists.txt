# Generated from windowflags.pro.

#####################################################################
## tst_manual_windowflags Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_windowflags
    GUI
    SOURCES
        controllerwindow.cpp controllerwindow.h
        controls.cpp controls.h
        main.cpp
        previewwindow.cpp previewwindow.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)
