# Generated from mimetypebrowser.pro.

#####################################################################
## mimetypebrowser Binary:
#####################################################################

add_qt_executable(mimetypebrowser
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/mimetypes/mimetypebrowser"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/mimetypes/mimetypebrowser"
    SOURCES
        main.cpp
        mainwindow.cpp mainwindow.h
        mimetypemodel.cpp mimetypemodel.h
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:mimetypebrowser.pro:<NONE>:
# CONFIG = "-app_bundle" "c++11"
# TEMPLATE = "app"
