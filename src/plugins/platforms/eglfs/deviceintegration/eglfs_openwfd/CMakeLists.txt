# Generated from openwfd.pro.

#####################################################################
## QEglFSOpenWFDIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QEglFSOpenWFDIntegrationPlugin
    OUTPUT_NAME qeglfs-openwfd-integration
    TYPE egldeviceintegrations
    SOURCES
        qeglfsopenwfdintegration.cpp qeglfsopenwfdintegration.h
        qeglfsopenwfdmain.cpp
    INCLUDE_DIRECTORIES
        ../../api
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::Gui
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:openwfd.pro:<TRUE>:
# OTHER_FILES = "$$PWD/eglfs_openwfd.json"
