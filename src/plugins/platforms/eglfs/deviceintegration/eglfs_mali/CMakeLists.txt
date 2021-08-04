# Generated from eglfs_mali.pro.

#####################################################################
## QEglFSMaliIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QEglFSMaliIntegrationPlugin
    OUTPUT_NAME qeglfs-mali-integration
    PLUGIN_TYPE egldeviceintegrations
    SOURCES
        qeglfsmaliintegration.cpp qeglfsmaliintegration.h
        qeglfsmalimain.cpp
    DEFINES
        QT_EGL_NO_X11
    INCLUDE_DIRECTORIES
        ../../api
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::Gui
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:eglfs_mali.pro:<TRUE>:
# OTHER_FILES = "$$PWD/eglfs_mali.json"
