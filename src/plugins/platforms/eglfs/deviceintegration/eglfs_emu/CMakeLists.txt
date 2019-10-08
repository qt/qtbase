# Generated from eglfs_emu.pro.

#####################################################################
## qeglfs-emu-integration Plugin:
#####################################################################

add_qt_plugin(qeglfs-emu-integration
    TYPE egldeviceintegrations
    CLASS_NAME QEglFSEmulatorIntegrationPlugin
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
# _LOADED = "qt_plugin"
