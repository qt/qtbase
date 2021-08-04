# Generated from eglfs_emu.pro.

#####################################################################
## QEglFSEmulatorIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QEglFSEmulatorIntegrationPlugin
    OUTPUT_NAME qeglfs-emu-integration
    PLUGIN_TYPE egldeviceintegrations
    SOURCES
        qeglfsemulatorintegration.cpp qeglfsemulatorintegration.h
        qeglfsemulatorscreen.cpp qeglfsemulatorscreen.h
        qeglfsemumain.cpp
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

#### Keys ignored in scope 1:.:.:eglfs_emu.pro:<TRUE>:
# DISTFILES = "eglfs_emu.json"
# OTHER_FILES = "$$PWD/eglfs_emu.json"
