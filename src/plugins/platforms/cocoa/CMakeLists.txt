find_package(OpenGL)
find_library(FWAppkit AppKit)
find_library(FWCoreServices CoreServices)
find_library(FWCarbon Carbon)
find_library(FWIOKit IOKit)
find_library(FWQuartzCore QuartzCore)
find_library(FWCoreVideo CoreVideo)
find_library(FWMetal Metal)
find_library(FWcups cups)

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
        qmacclipboard.h qmacclipboard.mm
        qmultitouch_mac.mm qmultitouch_mac_p.h
        qnsview.h qnsview.mm
        qnswindow.h qnswindow.mm
        qnswindowdelegate.h qnswindowdelegate.mm
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::AccessibilitySupportPrivate
        Qt::ClipboardSupportPrivate
        Qt::ThemeSupportPrivate
        Qt::FontDatabaseSupportPrivate
        Qt::GraphicsSupportPrivate
        Qt::PlatformHeaders
        ${FWAppKit}
        ${FWCoreServices}
        ${FWCarbon}
        ${FWIOKit}
        ${FWQuartzCore}
        ${FWCoreVideo}
        ${FWMetal}
        ${FWcups}
)

add_qt_resource(qcocoa qcocoaresources PREFIX "/qt-project.org/mac/cursors"
    FILES images/spincursor.png images/waitcursor.png images/sizeallcursor.png
)

extend_target(qcocoa CONDITION QT_FEATURE_opengl
    SOURCES
        qcocoaglcontext.h qcocoaglcontext.mm
    LIBRARIES
        OpenGL::GL
)

extend_target(qcocoa CONDITION QT_FEATURE_vulkan
    SOURCES
        qcocoavulkaninstance.h qcocoavulkaninstance.mm
)

extend_target(qcocoa CONDITION QT_FEATURE_vulkan
    LIBRARIES
        vulkan_support_private
)

extend_target(qcocoa CONDITION QT_FEATURE_widgets
    SOURCES
        qcocoaprintdevice.h qcocoaprintdevice.mm
        qcocoaprintersupport.h qcocoaprintersupport.mm
        qpaintengine_mac.mm qpaintengine_mac_p.h
        qprintengine_mac.mm qprintengine_mac_p.h
    LIBRARIES
        Qt::WidgetsPrivate
        Qt::PrintSupportPrivate
)

extend_target(qcocoa CONDITION (QT_FEATURE_widgets) AND (QT_FEATURE_colordialog)
    SOURCES
        qcocoacolordialoghelper.h qcocoacolordialoghelper.mm
)

extend_target(qcocoa CONDITION (QT_FEATURE_widgets) AND (QT_FEATURE_filedialog)
    SOURCES
        qcocoafiledialoghelper.h qcocoafiledialoghelper.mm
)

extend_target(qcocoa CONDITION (QT_FEATURE_widgets) AND (QT_FEATURE_fontdialog)
    SOURCES
        qcocoafontdialoghelper.h qcocoafontdialoghelper.mm
)

#extend_target(qcocoa CONDITION NOT equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN)
#)
