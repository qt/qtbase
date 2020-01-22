# Generated from eglfs_emu.pro.

#####################################################################
## QEglFSEmulatorIntegrationPlugin Plugin:
#####################################################################

qt_add_plugin(QEglFSEmulatorIntegrationPlugin
    OUTPUT_NAME qeglfs-emu-integration
    TYPE egldeviceintegrations
    SOURCES
        qeglfsemulatorintegration.cpp qeglfsemulatorintegration.h
        qeglfsemulatorscreen.cpp qeglfsemulatorscreen.h
        qeglfsemumain.cpp
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
)

#### Keys ignored in scope 1:.:.:eglfs_emu.pro:<TRUE>:
# DISTFILES = "eglfs_emu.json"
# OTHER_FILES = "$$PWD/eglfs_emu.json"
