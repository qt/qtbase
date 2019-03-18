# Generated from download.pro.

#####################################################################
## download Binary:
#####################################################################

add_qt_executable(download
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/download"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/download"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:download.pro:<NONE>:
# CONFIG = "-app_bundle"
