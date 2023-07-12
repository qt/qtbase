# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_find_package(EGL)

if(QT_FEATURE_eglfs_gbm)
    set(_device_integration "eglfs_kms")
elseif(QT_FEATURE_eglfs_egldevice)
    set(_device_integration "eglfs_kms_egldevice")
elseif(QT_FEATURE_eglfs_viv)
    set(_device_integration "eglfs_viv")
elseif(QT_FEATURE_eglfs_brcm)
    set(_device_integration "eglfs_brcm")
elseif(QT_FEATURE_eglfs_openwfd)
    set(_device_integration "eglfs_openwfd")
endif()

set(QT_QPA_DEFAULT_EGLFS_INTEGRATION "${_device_integration}" CACHE STRING "Default EGLFS device integration plugin")

#####################################################################
## EglFSDeviceIntegrationPrivate Module:
#####################################################################

qt_internal_add_module(EglFSDeviceIntegrationPrivate
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
        QT_NO_FOREACH
        EGLFS_PREFERRED_PLUGIN=${QT_QPA_DEFAULT_EGLFS_INTEGRATION}
    INCLUDE_DIRECTORIES
        api
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::DeviceDiscoverySupportPrivate
        Qt::FbSupportPrivate
        Qt::GuiPrivate
        EGL::EGL
    HEADER_SYNC_SOURCE_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/api"
)

## Scopes:
#####################################################################

qt_internal_extend_target(EglFSDeviceIntegrationPrivate CONDITION TARGET Qt::InputSupportPrivate
    PUBLIC_LIBRARIES
        Qt::InputSupportPrivate
)

qt_internal_extend_target(EglFSDeviceIntegrationPrivate CONDITION INTEGRITY AND TARGET IntegrityPlatformGraphics::IntegrityPlatformGraphics
    LIBRARIES
        IntegrityPlatformGraphics::IntegrityPlatformGraphics
)

qt_internal_extend_target(EglFSDeviceIntegrationPrivate CONDITION QT_FEATURE_opengl
    SOURCES
        api/qeglfscontext.cpp api/qeglfscontext_p.h
        api/qeglfscursor.cpp api/qeglfscursor_p.h
    PUBLIC_LIBRARIES
        Qt::OpenGLPrivate
)

if(QT_FEATURE_cursor)
    # Resources:
    set(cursor_resource_files
        "cursor-atlas.png"
        "cursor.json"
    )

    qt_internal_add_resource(EglFSDeviceIntegrationPrivate "cursor"
        PREFIX
            "/"
        FILES
            ${cursor_resource_files}
    )
endif()

#####################################################################
## QEglFSIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QEglFSIntegrationPlugin
    OUTPUT_NAME qeglfs
    PLUGIN_TYPE platforms
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES eglfs
    SOURCES
        qeglfsmain.cpp
    DEFINES
        QT_EGL_NO_X11
    LIBRARIES
        Qt::CorePrivate
        Qt::EglFSDeviceIntegrationPrivate
        EGL::EGL
)

add_subdirectory(deviceintegration)
