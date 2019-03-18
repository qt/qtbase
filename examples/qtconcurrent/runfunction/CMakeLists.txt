# Generated from runfunction.pro.

#####################################################################
## runfunction Binary:
#####################################################################

add_qt_executable(runfunction
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtconcurrent/runfunction"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtconcurrent/runfunction"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Concurrent
        Qt::Widgets
)

#### Keys ignored in scope 1:.:runfunction.pro:<NONE>:
# CONFIG = "-app_bundle"
