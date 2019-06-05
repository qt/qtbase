# Generated from eglfs_x11.pro.

#####################################################################
## qeglfs-x11-integration Plugin:
#####################################################################

add_qt_plugin(qeglfs-x11-integration
    TYPE egldeviceintegrations
    CLASS_NAME QEglFSX11IntegrationPlugin
    SOURCES
        qeglfsx11integration.cpp qeglfsx11integration.h
        qeglfsx11main.cpp
    DEFINES
        QT_EGL_NO_X11
    INCLUDE_DIRECTORIES
        ../../api
    LIBRARIES
        Qt::CorePrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::EglFSDeviceIntegration
        Qt::Gui
        X11::XCB
        XCB::XCB
)

#### Keys ignored in scope 1:.:.:eglfs_x11.pro:<TRUE>:
# CONFIG = "egl"
# OTHER_FILES = "$$PWD/eglfs_x11.json"
# PLUGIN_CLASS_NAME = "QEglFSX11IntegrationPlugin"
# PLUGIN_TYPE = "egldeviceintegrations"
# _LOADED = "qt_plugin"
