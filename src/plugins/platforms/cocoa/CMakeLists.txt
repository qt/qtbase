# Generated from cocoa.pro.

# special case:
qt_find_package(Cups PROVIDED_TARGETS Cups::Cups)

#####################################################################
## qcocoa Plugin:
#####################################################################

add_qt_plugin(qcocoa
    TYPE platforms
    SOURCES
        main.mm
        messages.cpp messages.h
        qcocoaaccessibility.h qcocoaaccessibility.mm
        qcocoaaccessibilityelement.h qcocoaaccessibilityelement.mm
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
        qcocoasystemsettings.h qcocoasystemsettings.mm
        qcocoasystemtrayicon.h qcocoasystemtrayicon.mm
        qcocoatheme.h qcocoatheme.mm
        qcocoawindow.h qcocoawindow.mm
        qiosurfacegraphicsbuffer.h qiosurfacegraphicsbuffer.mm
        qmacclipboard.h qmacclipboard.mm
        qmultitouch_mac.mm qmultitouch_mac_p.h
        qnsview.h qnsview.mm
        qnswindow.h qnswindow.mm
        qnswindowdelegate.h qnswindowdelegate.mm
    LIBRARIES
        Qt::AccessibilitySupportPrivate
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
        Qt::AccessibilitySupport
        Qt::ClipboardSupport
        Qt::Core
        Qt::FontDatabaseSupport
        Qt::GraphicsSupport
        Qt::Gui
        Qt::ThemeSupport
)

# Resources:
add_qt_resource(qcocoa "qcocoaresources" PREFIX "/qt-project.org/mac/cursors" FILES
    images/sizeallcursor.png
    images/spincursor.png
    images/waitcursor.png)


#### Keys ignored in scope 1:.:.:cocoa.pro:<TRUE>:
# CONFIG = "no_app_extension_api_only"
# OTHER_FILES = "cocoa.json"
# PLUGIN_CLASS_NAME = "QCocoaIntegrationPlugin"
# _LOADED = "qt_plugin"

## Scopes:
#####################################################################

extend_target(qcocoa CONDITION QT_FEATURE_opengl # special case
    SOURCES
        qcocoaglcontext.h qcocoaglcontext.mm
)

extend_target(qcocoa CONDITION QT_FEATURE_vulkan
    SOURCES
        qcocoavulkaninstance.h qcocoavulkaninstance.mm
    LIBRARIES
        Qt::VulkanSupportPrivate
    PUBLIC_LIBRARIES
        Qt::VulkanSupport
)

extend_target(qcocoa CONDITION TARGET Qt::Widgets
    SOURCES
        qcocoaprintdevice.h qcocoaprintdevice.mm
        qcocoaprintersupport.h qcocoaprintersupport.mm
        qpaintengine_mac.mm qpaintengine_mac_p.h
        qprintengine_mac.mm qprintengine_mac_p.h
    LIBRARIES
        Qt::PrintSupportPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::PrintSupport
        Qt::Widgets
)

#### Keys ignored in scope 5:.:.:cocoa.pro:TARGET Qt::Widgets:
# QT_FOR_CONFIG = "widgets"

extend_target(qcocoa CONDITION (TARGET Qt::Widgets) AND (QT_FEATURE_colordialog)
    SOURCES
        qcocoacolordialoghelper.h qcocoacolordialoghelper.mm
)

extend_target(qcocoa CONDITION (TARGET Qt::Widgets) AND (QT_FEATURE_filedialog)
    SOURCES
        qcocoafiledialoghelper.h qcocoafiledialoghelper.mm
)

extend_target(qcocoa CONDITION (TARGET Qt::Widgets) AND (QT_FEATURE_fontdialog)
    SOURCES
        qcocoafontdialoghelper.h qcocoafontdialoghelper.mm
)

#### Keys ignored in scope 9:.:.:cocoa.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"
