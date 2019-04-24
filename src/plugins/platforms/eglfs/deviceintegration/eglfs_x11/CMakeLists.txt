# Generated from eglfs_x11.pro.

qt_find_package(XCB)
qt_find_package(X11)
qt_find_package(X11_XCB)

#####################################################################
## qeglfs-x11-integration Plugin:
#####################################################################

add_qt_plugin(qeglfs-x11-integration
    TYPE egldeviceintegrations
    SOURCES
        qeglfsx11integration.cpp qeglfsx11integration.h
        qeglfsx11main.cpp
    DEFINES
        QT_EGL_NO_X11
    INCLUDE_DIRECTORIES
        ../../api
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::EglFSDeviceIntegrationPrivate
        XCB::XCB
        X11::X11
        X11::XCB
    # CONFIG = "egl"
    # OTHER_FILES = "$$PWD/eglfs_x11.json"
    # PLUGIN_CLASS_NAME = "QEglFSX11IntegrationPlugin"
    # _LOADED = "qt_plugin"
)
