# Generated from cocoa.pro.

# special case:
qt_find_package(Cups PROVIDED_TARGETS Cups::Cups)

#####################################################################
## QCocoaIntegrationPlugin Plugin:
#####################################################################

qt_add_plugin(QCocoaIntegrationPlugin
    OUTPUT_NAME qcocoa
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES cocoa # special case
    TYPE platforms
    SOURCES
        main.mm
        messages.cpp messages.h
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
        qcocoakeymapper.h qcocoakeymapper.mm
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
    LIBRARIES
        Qt::ClipboardSupportPrivate
        Qt::CorePrivate
        Qt::FontDatabaseSupportPrivate
        Qt::GraphicsSupportPrivate
        Qt::GuiPrivate
        Qt::ThemeSupportPrivate
    PUBLIC_LIBRARIES
        ${FWAppKit}
        ${FWCarbon}
        ${FWCoreServices}
        ${FWCoreVideo}
        ${FWIOKit}
        ${FWIOSurface}
        ${FWMetal}
        ${FWQuartzCore}
        Cups::Cups
        Qt::ClipboardSupport
        Qt::Core
        Qt::FontDatabaseSupport
        Qt::GraphicsSupport
        Qt::Gui
        Qt::ThemeSupport
)

# Resources:
set(qcocoaresources_resource_files
    "images/sizeallcursor.png"
    "images/spincursor.png"
    "images/waitcursor.png"
)

qt_add_resource(QCocoaIntegrationPlugin "qcocoaresources"
    PREFIX
        "/qt-project.org/mac/cursors"
    FILES
        ${qcocoaresources_resource_files}
)


#### Keys ignored in scope 1:.:.:cocoa.pro:<TRUE>:
# OTHER_FILES = "cocoa.json"

## Scopes:
#####################################################################

qt_extend_target(QCocoaIntegrationPlugin CONDITION QT_FEATURE_opengl
    SOURCES
        qcocoaglcontext.h qcocoaglcontext.mm
)

qt_extend_target(QCocoaIntegrationPlugin CONDITION QT_FEATURE_vulkan
    SOURCES
        qcocoavulkaninstance.h qcocoavulkaninstance.mm
    LIBRARIES
        Qt::VulkanSupportPrivate
    PUBLIC_LIBRARIES
        Qt::VulkanSupport
)

qt_extend_target(QCocoaIntegrationPlugin CONDITION QT_FEATURE_accessibility
    SOURCES
        qcocoaaccessibility.h qcocoaaccessibility.mm
        qcocoaaccessibilityelement.h qcocoaaccessibilityelement.mm
    LIBRARIES
        Qt::AccessibilitySupportPrivate
    PUBLIC_LIBRARIES
        Qt::AccessibilitySupport
)

qt_extend_target(QCocoaIntegrationPlugin CONDITION QT_FEATURE_sessionmanager
    SOURCES
        qcocoasessionmanager.cpp qcocoasessionmanager.h
)

qt_extend_target(QCocoaIntegrationPlugin CONDITION TARGET Qt::Widgets
    SOURCES
        qpaintengine_mac.mm qpaintengine_mac_p.h
    LIBRARIES
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 7:.:.:cocoa.pro:TARGET Qt::Widgets:
# QT_FOR_CONFIG = "widgets"

qt_extend_target(QCocoaIntegrationPlugin CONDITION TARGET Qt::PrintSupport AND TARGET Qt::Widgets
    SOURCES
        qcocoaprintdevice.h qcocoaprintdevice.mm
        qcocoaprintersupport.h qcocoaprintersupport.mm
        qprintengine_mac.mm qprintengine_mac_p.h
    LIBRARIES
        Qt::PrintSupportPrivate
    PUBLIC_LIBRARIES
        Qt::PrintSupport
)

qt_extend_target(QCocoaIntegrationPlugin CONDITION QT_FEATURE_colordialog AND TARGET Qt::Widgets
    SOURCES
        qcocoacolordialoghelper.h qcocoacolordialoghelper.mm
)

qt_extend_target(QCocoaIntegrationPlugin CONDITION QT_FEATURE_filedialog AND TARGET Qt::Widgets
    SOURCES
        qcocoafiledialoghelper.h qcocoafiledialoghelper.mm
)

qt_extend_target(QCocoaIntegrationPlugin CONDITION QT_FEATURE_fontdialog AND TARGET Qt::Widgets
    SOURCES
        qcocoafontdialoghelper.h qcocoafontdialoghelper.mm
)

#### Keys ignored in scope 12:.:.:cocoa.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"
