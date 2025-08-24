# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from src.pro.
# special case begin

set(QT_SBOM_DEFAULT_QT_LICENSE_ID_LIBRARIES "QT_COMMERCIAL_OR_LGPL3")

if (NOT QT_FEATURE_waylandscanner OR NOT QT_FEATURE_wayland)
    return()
endif()

# See globalprivate/README for a description of the following module.
qt_internal_add_module(WaylandGlobalPrivate
    INTERNAL_MODULE
    HEADER_MODULE
    NO_GENERATE_CPP_EXPORTS
)

# Work around 115101.
# If nothing depends on the WaylandGlobalPrivate target it doesn't run custom commands that the
# target depends on. WaylandGlobalPrivate_ensure_sync_headers makes sure that 'all' depends on
# WaylandGlobalPrivate_sync_headers.
# TODO: This needs to be removed once the fix for QTBUG-115101 is merged in qtbase.
add_custom_target(WaylandGlobalPrivate_ensure_sync_headers ALL)
add_dependencies(WaylandGlobalPrivate_ensure_sync_headers WaylandGlobalPrivate_sync_headers)

# special case begin
# TODO: Ideally these macros would be part of the qtwaylandscanner tool, and not the compositor/client
include(../../../../src/tools/qtwaylandscanner/Qt6WaylandClientMacros.cmake)
# special case end

# special case end

# Generated from client.pro.

#####################################################################
## WaylandClient Module:
#####################################################################

qt_internal_add_module(WaylandClient
    PLUGIN_TYPES wayland-graphics-integration-client wayland-inputdevice-integration wayland-decoration-client wayland-shell-integration
    SOURCES
        shared/qwaylandinputmethodeventbuilder.cpp shared/qwaylandinputmethodeventbuilder_p.h
        shared/qwaylandmimehelper.cpp shared/qwaylandmimehelper_p.h
        shared/qwaylandsharedmemoryformathelper_p.h
        global/qwaylandclientextension.cpp global/qwaylandclientextension.h global/qwaylandclientextension_p.h
        hardwareintegration/qwaylandclientbufferintegration.cpp hardwareintegration/qwaylandclientbufferintegration_p.h
        hardwareintegration/qwaylandclientbufferintegrationfactory.cpp hardwareintegration/qwaylandclientbufferintegrationfactory_p.h
        hardwareintegration/qwaylandclientbufferintegrationplugin.cpp hardwareintegration/qwaylandclientbufferintegrationplugin_p.h
        hardwareintegration/qwaylandhardwareintegration.cpp hardwareintegration/qwaylandhardwareintegration_p.h
        hardwareintegration/qwaylandserverbufferintegration.cpp hardwareintegration/qwaylandserverbufferintegration_p.h
        hardwareintegration/qwaylandserverbufferintegrationfactory.cpp hardwareintegration/qwaylandserverbufferintegrationfactory_p.h
        hardwareintegration/qwaylandserverbufferintegrationplugin.cpp hardwareintegration/qwaylandserverbufferintegrationplugin_p.h
        inputdeviceintegration/qwaylandinputdeviceintegration_p.h
        inputdeviceintegration/qwaylandinputdeviceintegrationfactory.cpp inputdeviceintegration/qwaylandinputdeviceintegrationfactory_p.h
        inputdeviceintegration/qwaylandinputdeviceintegrationplugin.cpp inputdeviceintegration/qwaylandinputdeviceintegrationplugin_p.h
        qtwaylandclientglobal.h qtwaylandclientglobal_p.h
        qwaylandabstractdecoration.cpp qwaylandabstractdecoration_p.h
        qwaylandappmenu.cpp qwaylandappmenu_p.h
        qwaylandbuffer.cpp qwaylandbuffer_p.h
        qwaylandcolormanagement.cpp qwaylandcolormanagement_p.h
        qwaylanddatacontrolv1.cpp qwaylanddatacontrolv1_p.h
        qwaylanddecorationfactory.cpp qwaylanddecorationfactory_p.h
        qwaylanddecorationplugin.cpp qwaylanddecorationplugin_p.h
        qwaylanddisplay.cpp qwaylanddisplay_p.h
        qwaylandfractionalscale.cpp qwaylandfractionalscale_p.h
        qwaylandinputcontext.cpp qwaylandinputcontext_p.h
        qwaylandtextinputv1.cpp qwaylandtextinputv1_p.h
        qwaylandtextinputv2.cpp qwaylandtextinputv2_p.h
        qwaylandtextinputv3.cpp qwaylandtextinputv3_p.h
        qwaylandtextinputinterface.cpp qwaylandtextinputinterface_p.h
        qwaylandinputdevice.cpp qwaylandinputdevice_p.h
        qwaylandinputmethodcontext.cpp qwaylandinputmethodcontext_p.h
        qwaylandintegration.cpp qwaylandintegration_p.h
        qwaylandnativeinterface.cpp qwaylandnativeinterface_p.h
        qwaylandplatformservices.cpp qwaylandplatformservices_p.h
        qwaylandpointergestures.cpp qwaylandpointergestures_p.h
        qwaylandscreen.cpp qwaylandscreen_p.h
        qwaylandshellsurface.cpp qwaylandshellsurface_p.h
        qwaylandshm.cpp qwaylandshm_p.h
        qwaylandshmbackingstore.cpp qwaylandshmbackingstore_p.h
        qwaylandshmwindow.cpp qwaylandshmwindow_p.h
        qwaylandsubsurface.cpp qwaylandsubsurface_p.h
        qwaylandsurface.cpp qwaylandsurface_p.h
        qwaylandviewport.cpp qwaylandviewport_p.h
        qwaylandwindow.cpp qwaylandwindow_p.h
        qwaylandwindowmanagerintegration.cpp qwaylandwindowmanagerintegration_p.h
        qwaylandeventdispatcher.cpp qwaylandeventdispatcher_p.h
        shellintegration/qwaylandclientshellapi_p.h
        shellintegration/qwaylandshellintegration_p.h shellintegration/qwaylandshellintegration.cpp
        shellintegration/qwaylandshellintegrationfactory.cpp shellintegration/qwaylandshellintegrationfactory_p.h
        shellintegration/qwaylandshellintegrationplugin.cpp shellintegration/qwaylandshellintegrationplugin_p.h
    INCLUDE_DIRECTORIES
        shared
        global
        hardwareintegration
        inputdeviceintegration
        shellintegration
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WaylandGlobalPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Wayland::Client
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WaylandGlobalPrivate
    PRIVATE_HEADER_FILTERS
        "^qwayland-.*\.h|^wayland-.*-protocol\.h"
    QT_LICENSE_ID QT_COMMERCIAL_OR_LGPL3
    ATTRIBUTION_FILE_DIR_PATHS
        ../../../3rdparty/wayland/protocols/pointer-gestures
        ../../../3rdparty/wayland/protocols/tablet
        ../../../3rdparty/wayland/protocols/text-input/v1
        ../../../3rdparty/wayland/protocols/text-input/v2
        ../../../3rdparty/wayland/protocols/text-input/v3
        ../../../3rdparty/wayland/protocols/wayland
        ../../../3rdparty/wayland/protocols/wp-primary-selection
        ../../../3rdparty/wayland/protocols/xdg-output
        ../../../3rdparty/wayland/protocols/fractional-scale
        ../../../3rdparty/wayland/protocols/viewporter
        ../../../3rdparty/wayland/protocols/xdg-shell
        ../../../3rdparty/wayland/protocols/wlr-data-control
)

qt_internal_add_plugin(QWaylandIntegrationPlugin
    OUTPUT_NAME qwayland
    PLUGIN_TYPE platforms
    DEFAULT_IF "wayland" IN_LIST QT_QPA_PLATFORMS
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::WaylandClientPrivate
    QT_LICENSE_ID QT_COMMERCIAL_OR_LGPL3
)

qt6_generate_wayland_protocol_client_sources(WaylandClient
    PRIVATE_CODE
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/appmenu/appmenu.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/cursor-shape/cursor-shape-v1.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/pointer-gestures/pointer-gestures-unstable-v1.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/tablet/tablet-unstable-v2.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/text-input/v1/text-input-unstable-v1.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/text-input/v2/text-input-unstable-v2.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/text-input/v3/text-input-unstable-v3.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/wayland/wayland.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/wp-primary-selection/wp-primary-selection-unstable-v1.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/xdg-output/xdg-output-unstable-v1.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/fractional-scale/fractional-scale-v1.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/viewporter/viewporter.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/wlr-data-control/wlr-data-control-unstable-v1.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/xdg-shell/xdg-shell.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/xdg-system-bell/xdg-system-bell-v1.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/xdg-toplevel-drag/xdg-toplevel-drag-v1.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/extensions/qt-text-input-method-unstable-v1.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/extensions/qt-windowmanager.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/extensions/hardware-integration.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/extensions/server-buffer-extension.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/color-management/xx-color-management-v4.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/wayland/protocols/pointer-warp/pointer-warp-v1.xml
)

#### Keys ignored in scope 1:.:.:client.pro:<TRUE>:
# MODULE = "waylandclient"
# QMAKE_CXXFLAGS_WARN_ON = "--Wcast-qual"

## Scopes:
#####################################################################

# special case begin
#
# Do not explicitly add XKB::XKB. This is already done by Qt::GuiPrivate.
#
# qt_internal_extend_target(WaylandClient CONDITION QT_FEATURE_xkbcommon
#     PUBLIC_LIBRARIES
#         XKB::XKB
# )
# special case end

qt_internal_extend_target(WaylandClient CONDITION QT_FEATURE_tabletevent
    SOURCES
        qwaylandtabletv2.cpp qwaylandtabletv2_p.h
)

qt_internal_extend_target(WaylandClient CONDITION QT_FEATURE_clipboard
    SOURCES
        qwaylandclipboard.cpp qwaylandclipboard_p.h
)

qt_internal_extend_target(WaylandClient CONDITION QT_FEATURE_vulkan
    SOURCES
        qwaylandvulkaninstance.cpp qwaylandvulkaninstance_p.h
        qwaylandvulkanwindow.cpp qwaylandvulkanwindow_p.h
)

qt_internal_extend_target(WaylandClient CONDITION QT_FEATURE_cursor
    SOURCES
        qwaylandcursor.cpp qwaylandcursor_p.h
    PUBLIC_LIBRARIES
        Wayland::Cursor
)

qt_internal_extend_target(WaylandClient CONDITION QT_FEATURE_wayland_datadevice
    SOURCES
        qwaylanddatadevice.cpp qwaylanddatadevice_p.h
        qwaylanddatadevicemanager.cpp qwaylanddatadevicemanager_p.h
        qwaylanddataoffer.cpp qwaylanddataoffer_p.h
        qwaylanddatasource.cpp qwaylanddatasource_p.h
)

qt_internal_extend_target(WaylandClient CONDITION QT_FEATURE_wayland_client_primary_selection
    SOURCES
        qwaylandprimaryselectionv1.cpp qwaylandprimaryselectionv1_p.h
)

qt_internal_extend_target(WaylandClient CONDITION QT_FEATURE_draganddrop
    SOURCES
        qwaylanddnd.cpp qwaylanddnd_p.h
)

qt_internal_extend_target(WaylandClient CONDITION QT_FEATURE_egl AND NOT QT_FEATURE_egl_x11
    DEFINES
        QT_EGL_NO_X11
)

qt_internal_add_docs(WaylandClient
    doc/qtwaylandclient.qdocconf
)

qt_record_extra_qt_main_tools_package_dependency(WaylandClient WaylandScannerTools "${PROJECT_VERSION}")
qt_internal_register_target_dependencies(WaylandClient PUBLIC Qt6::WaylandGlobalPrivate)

if (QT_FEATURE_wayland_client)
    add_subdirectory(plugins)
endif()
