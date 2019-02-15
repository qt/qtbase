# Generated from eglfs_emu.pro.

#####################################################################
## qeglfs-emu-integration Plugin:
#####################################################################

add_qt_plugin(qeglfs-emu-integration
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
        Qt::GuiPrivate
        Qt::EglFSDeviceIntegrationPrivate
    # CONFIG = "egl"
    # DISTFILES = "eglfs_emu.json"
    # OTHER_FILES = "$$PWD/eglfs_emu.json"
    # PLUGIN_CLASS_NAME = "QEglFSEmulatorIntegrationPlugin"
    # _LOADED = "qt_plugin"
)
