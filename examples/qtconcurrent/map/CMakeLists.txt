# Generated from map.pro.

#####################################################################
## mapdemo Binary:
#####################################################################

add_qt_executable(mapdemo
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtconcurrent/map"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtconcurrent/map"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Concurrent
        Qt::Gui # special case
)

#### Keys ignored in scope 1:.:map.pro:<NONE>:
# CONFIG = "-app_bundle"
# TEMPLATE = "app"
