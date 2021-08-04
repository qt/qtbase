# Generated from eglfs_viv.pro.

#####################################################################
## QEglFSVivIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QEglFSVivIntegrationPlugin
    OUTPUT_NAME qeglfs-viv-integration
    PLUGIN_TYPE egldeviceintegrations
    SOURCES
        qeglfsvivintegration.cpp qeglfsvivintegration.h
        qeglfsvivmain.cpp
    DEFINES
        EGL_API_FB=1
        LINUX=1
    INCLUDE_DIRECTORIES
        ../../api
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::Gui
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:eglfs_viv.pro:<TRUE>:
# OTHER_FILES = "$$PWD/eglfs_viv.json"
