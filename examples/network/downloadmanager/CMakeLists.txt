# Generated from downloadmanager.pro.

#####################################################################
## downloadmanager Binary:
#####################################################################

add_qt_executable(downloadmanager
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/downloadmanager"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/downloadmanager"
    SOURCES
        downloadmanager.cpp downloadmanager.h
        main.cpp
        textprogressbar.cpp textprogressbar.h
    LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:downloadmanager.pro:<NONE>:
# CONFIG = "-app_bundle"
# OTHER_FILES = "debian/changelog" "debian/compat" "debian/control" "debian/copyright" "debian/README" "debian/rules"
