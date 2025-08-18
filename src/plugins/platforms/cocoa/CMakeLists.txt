# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QCocoaIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QCocoaIntegrationPlugin
    OUTPUT_NAME qcocoa
    DEFAULT_IF "cocoa" IN_LIST QT_QPA_PLATFORMS
    PLUGIN_TYPE platforms
    SOURCES
        main.mm
        qcocoaapplication.h qcocoaapplication.mm
        qcocoaapplicationdelegate.h qcocoaapplicationdelegate.mm
        qcocoabackingstore.h qcocoabackingstore.mm
        qcocoaclipboard.h qcocoaclipboard.mm
        qcocoacursor.h qcocoacursor.mm
        qcocoadrag.h qcocoadrag.mm
        qcocoaeventdispatcher.h qcocoaeventdispatcher.mm
        qcocoahelpers.h qcocoahelpers.mm
        qcocoainputcontext.h qcocoainputcontext.mm
        qcocoaintegration.h qcocoaintegration.mm
        qcocoaintrospection.h qcocoaintrospection.mm
        qcocoamenu.h qcocoamenu.mm
        qcocoamenubar.h qcocoamenubar.mm
        qcocoamenuitem.h qcocoamenuitem.mm
        qcocoamenuloader.h qcocoamenuloader.mm
        qcocoamimetypes.h qcocoamimetypes.mm
        qcocoanativeinterface.h qcocoanativeinterface.mm
        qcocoansmenu.h qcocoansmenu.mm
        qcocoascreen.h qcocoascreen.mm
        qcocoaservices.h qcocoaservices.mm
        qcocoasystemtrayicon.h qcocoasystemtrayicon.mm
        qcocoatheme.h qcocoatheme.mm
        qcocoawindow.h qcocoawindow.mm
        qcocoawindowmanager.h qcocoawindowmanager.mm
        qiosurfacegraphicsbuffer.h qiosurfacegraphicsbuffer.mm
        qmacclipboard.h qmacclipboard.mm
        qmultitouch_mac.mm qmultitouch_mac_p.h
        qnsview.h qnsview.mm
        qnsview_drawing.mm
        qnsview_mouse.mm
        qnsview_touch.mm
        qnsview_gestures.mm
        qnsview_tablet.mm
        qnsview_dragging.mm
        qnsview_keys.mm
        qnsview_complextext.mm
        qnsview_menus.mm
        qnsview_accessibility.mm
        qnswindow.h qnswindow.mm
        qnswindowdelegate.h qnswindowdelegate.mm
        qcocoacolordialoghelper.h qcocoacolordialoghelper.mm
        qcocoafiledialoghelper.h qcocoafiledialoghelper.mm
        qcocoafontdialoghelper.h qcocoafontdialoghelper.mm
        qcocoamessagedialog.h qcocoamessagedialog.mm
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        ${FWFoundation}
        ${FWAppKit}
        ${FWCarbon}
        ${FWCoreServices}
        ${FWCoreVideo}
        ${FWIOKit}
        ${FWIOSurface}
        ${FWMetal}
        ${FWQuartzCore}
        ${FWUniformTypeIdentifiers}
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

# compiled as part of qnsview.mm
set_property(SOURCE
    qnsview_drawing.mm
    qnsview_mouse.mm
    qnsview_touch.mm
    qnsview_gestures.mm
    qnsview_tablet.mm
    qnsview_dragging.mm
    qnsview_keys.mm
    qnsview_complextext.mm
    qnsview_menus.mm
    qnsview_accessibility.mm
    PROPERTY
        HEADER_FILE_ONLY TRUE
)

qt_disable_apple_app_extension_api_only(QCocoaIntegrationPlugin)

# Resources:
set(qcocoaresources_resource_files
    "images/sizeallcursor.png"
    "images/spincursor.png"
    "images/waitcursor.png"
)

qt_internal_add_resource(QCocoaIntegrationPlugin "qcocoaresources"
    PREFIX
        "/qt-project.org/mac/cursors"
    FILES
        ${qcocoaresources_resource_files}
)

if(CMAKE_OSX_DEPLOYMENT_TARGET VERSION_GREATER_EQUAL 15.0)
    message(FATAL_ERROR "QCocoaScreen uses APIs removed in 15.0. Please specify a
        CMAKE_OSX_DEPLOYMENT_TARGET of 14.0 or lower")
endif()

## Scopes:
#####################################################################

qt_internal_extend_target(QCocoaIntegrationPlugin CONDITION QT_FEATURE_opengl
    SOURCES
        qcocoaglcontext.h qcocoaglcontext.mm
)

qt_internal_extend_target(QCocoaIntegrationPlugin CONDITION QT_FEATURE_vulkan
    SOURCES
        qcocoavulkaninstance.h qcocoavulkaninstance.mm
)

qt_internal_extend_target(QCocoaIntegrationPlugin CONDITION QT_FEATURE_accessibility
    SOURCES
        qcocoaaccessibility.h qcocoaaccessibility.mm
        qcocoaaccessibilityelement.h qcocoaaccessibilityelement.mm
)

qt_internal_extend_target(QCocoaIntegrationPlugin CONDITION QT_FEATURE_sessionmanager
    SOURCES
        qcocoasessionmanager.cpp qcocoasessionmanager.h
)

# Work around CMake issue 23464
if(CMAKE_VERSION VERSION_LESS "3.25" AND NOT QT_FEATURE_sessionmanager)
    set_target_properties(QCocoaIntegrationPlugin PROPERTIES
        DISABLE_PRECOMPILE_HEADERS ON
    )
endif()
