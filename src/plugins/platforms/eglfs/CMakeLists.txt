# Generated from eglfs.pro.
qt_find_package(EGL) # special case

#####################################################################
## EglFSDeviceIntegration Module:
#####################################################################

add_qt_module(EglFSDeviceIntegration
    INTERNAL_MODULE
    SOURCES
        api/qeglfsdeviceintegration.cpp api/qeglfsdeviceintegration_p.h
        api/qeglfsglobal_p.h
        api/qeglfshooks.cpp api/qeglfshooks_p.h
        api/qeglfsintegration.cpp api/qeglfsintegration_p.h
        api/qeglfsoffscreenwindow.cpp api/qeglfsoffscreenwindow_p.h
        api/qeglfsscreen.cpp api/qeglfsscreen_p.h
        api/qeglfswindow.cpp api/qeglfswindow_p.h
    DEFINES
        QT_BUILD_EGL_DEVICE_LIB
        QT_EGL_NO_X11
    INCLUDE_DIRECTORIES
        api
    LIBRARIES
        Qt::CorePrivate
        Qt::DeviceDiscoverySupportPrivate
        Qt::EglSupportPrivate
        Qt::EventDispatcherSupportPrivate
        Qt::FbSupportPrivate
        Qt::FontDatabaseSupportPrivate
        Qt::GuiPrivate
        Qt::ServiceSupportPrivate
        Qt::ThemeSupportPrivate
        EGL::EGL # special case
)

#### Keys ignored in scope 2:.:.:eglfsdeviceintegration.pro:<TRUE>:
# CONFIG = "internal_module" "egl"
# MODULE = "eglfsdeviceintegration"
# _LOADED = "qt_module"

## Scopes:
#####################################################################

extend_target(EglFSDeviceIntegration CONDITION TARGET Qt::InputSupportPrivate
    LIBRARIES
        Qt::InputSupportPrivate
    PUBLIC_LIBRARIES
        Qt::InputSupport
)

extend_target(EglFSDeviceIntegration CONDITION TARGET Qt::PlatformCompositorSupportPrivate
    LIBRARIES
        Qt::PlatformCompositorSupportPrivate
    PUBLIC_LIBRARIES
        Qt::PlatformCompositorSupport
)

# special case:
# extend_target(EglFSDeviceIntegration CONDITION NOT EGLFS_PLATFORM_HOOKS_SOURCES_ISEMPTY ...
# extend_target(EglFSDeviceIntegration CONDITION NOT EGLFS_DEVICE_INTEGRATION_ISEMPTY ...

#### Keys ignored in scope 7:.:.:eglfsdeviceintegration.pro:use_gold_linker:
# CONFIG = "no_linker_version_script"

# Resources:
add_qt_resource(EglFSDeviceIntegration "cursor" FILES
    cursor-atlas.png
    cursor.json)


extend_target(EglFSDeviceIntegration CONDITION QT_FEATURE_opengl
    SOURCES
        api/qeglfscontext.cpp api/qeglfscontext_p.h
        api/qeglfscursor.cpp api/qeglfscursor_p.h
)
#####################################################################
## qeglfs Plugin:
#####################################################################

add_qt_plugin(qeglfs
    TYPE platforms
    CLASS_NAME QEglFSIntegrationPlugin
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES eglfs # special case
    SOURCES
        qeglfsmain.cpp
    DEFINES
        QT_EGL_NO_X11
    INCLUDE_DIRECTORIES
        api
    LIBRARIES
        Qt::CorePrivate # special case
        Qt::EglFSDeviceIntegrationPrivate
        EGL::EGL # special case
)

#### Keys ignored in scope 11:.:.:eglfs-plugin.pro:<TRUE>:
# CONFIG = "egl"
# OTHER_FILES = "$$PWD/eglfs.json"
# PLUGIN_CLASS_NAME = "QEglFSIntegrationPlugin"
# PLUGIN_TYPE = "platforms"
# _LOADED = "qt_plugin"

## Scopes:
#####################################################################

#### Keys ignored in scope 12:.:.:eglfs-plugin.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"
add_subdirectory(deviceintegration)

