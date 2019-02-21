# Generated from eglfs.pro.

find_package(EGL)
find_package(WrapOpenGL)

#####################################################################
## EglFSDeviceIntegration Module:
#####################################################################

add_qt_module(EglFSDeviceIntegration
    SOURCES
        api/qeglfsdeviceintegration.cpp api/qeglfsdeviceintegration_p.h
        api/qeglfsglobal_p.h
        api/qeglfshooks.cpp api/qeglfshooks_p.h
        api/qeglfsintegration.cpp api/qeglfsintegration_p.h
        api/qeglfsoffscreenwindow.cpp api/qeglfsoffscreenwindow_p.h
        api/qeglfsscreen.cpp api/qeglfsscreen_p.h
        api/qeglfswindow.cpp api/qeglfswindow_p.h
    DEFINES
        QT_EGL_NO_X11
        QT_BUILD_EGL_DEVICE_LIB
    INCLUDE_DIRECTORIES
        api
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::DeviceDiscoverySupportPrivate
        Qt::EventDispatcherSupportPrivate
        Qt::ServiceSupportPrivate
        Qt::ThemeSupportPrivate
        Qt::FontDatabaseSupportPrivate
        Qt::FbSupportPrivate
        Qt::EglSupportPrivate
        EGL::EGL
        WrapOpenGL
    # CONFIG = "internal_module" "egl"
    # MODULE = "eglfsdeviceintegration"
    # _LOADED = "qt_module"
)

## Scopes:
#####################################################################

extend_target(EglFSDeviceIntegration CONDITION TARGET Qt::InputSupportPrivate
    LIBRARIES
        Qt::InputSupportPrivate
)

extend_target(EglFSDeviceIntegration CONDITION TARGET Qt::PlatformCompositorSupportPrivate
    LIBRARIES
        Qt::PlatformCompositorSupportPrivate
)

# TODO
# extend_target(EglFSDeviceIntegration CONDITION NOT EGLFS_PLATFORM_HOOKS_SOURCES_ISEMPTY
#     DEFINES
#         EGLFS_PLATFORM_HOOKS
# )

# TODO
# extend_target(EglFSDeviceIntegration CONDITION NOT EGLFS_DEVICE_INTEGRATION_ISEMPTY
#     DEFINES
#         EGLFS_PREFERRED_PLUGIN=
# )
#
#extend_target(EglFSDeviceIntegration CONDITION use_gold_linker
#    # CONFIG = "no_linker_version_script"
#)

extend_target(EglFSDeviceIntegration CONDITION NOT DEFINES___contains___QT_NO_CURSOR
    SOURCES
        cursor.qrc
)

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
    SOURCES
        qeglfsmain.cpp
    DEFINES
        QT_EGL_NO_X11
    INCLUDE_DIRECTORIES
        api
    LIBRARIES
        Qt::CorePrivate
        Qt::EglFSDeviceIntegrationPrivate
        EGL::EGL
    # CONFIG = "egl"
    # OTHER_FILES = "$$PWD/eglfs.json"
    # PLUGIN_CLASS_NAME = "QEglFSIntegrationPlugin"
    # _LOADED = "qt_plugin"
)

## Scopes:
#####################################################################
#
#extend_target(qeglfs CONDITION NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN
#    # PLUGIN_EXTENDS = "-"
#)

add_subdirectory(deviceintegration)
