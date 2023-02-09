# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QCocoaIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QCocoaIntegrationPlugin
    OUTPUT_NAME qcocoa
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES cocoa
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
        qnswindow.h qnswindow.mm
        qnswindowdelegate.h qnswindowdelegate.mm
        qcocoacolordialoghelper.h qcocoacolordialoghelper.mm
        qcocoafiledialoghelper.h qcocoafiledialoghelper.mm
        qcocoafontdialoghelper.h qcocoafontdialoghelper.mm
        qcocoamessagedialog.h qcocoamessagedialog.mm
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        ${FWAppKit}
        ${FWCarbon}
        ${FWCoreServices}
        ${FWCoreVideo}
        ${FWIOKit}
        ${FWIOSurface}
        ${FWMetal}
        ${FWQuartzCore}
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
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
