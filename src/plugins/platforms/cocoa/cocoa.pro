TARGET = qcocoa

SOURCES += main.mm \
    qcocoaintegration.mm \
    qcocoascreen.mm \
    qcocoatheme.mm \
    qcocoabackingstore.mm \
    qcocoawindow.mm \
    qcocoawindowmanager.mm \
    qnsview.mm \
    qnswindow.mm \
    qnswindowdelegate.mm \
    qcocoanativeinterface.mm \
    qcocoaeventdispatcher.mm \
    qcocoaapplicationdelegate.mm \
    qcocoaapplication.mm \
    qcocoansmenu.mm \
    qcocoamenu.mm \
    qcocoamenuitem.mm \
    qcocoamenubar.mm \
    qcocoamenuloader.mm \
    qcocoahelpers.mm \
    qmultitouch_mac.mm \
    qcocoacursor.mm \
    qcocoaclipboard.mm \
    qcocoadrag.mm \
    qmacclipboard.mm \
    qcocoainputcontext.mm \
    qcocoaservices.mm \
    qcocoasystemtrayicon.mm \
    qcocoaintrospection.mm \
    qcocoakeymapper.mm \
    qcocoamimetypes.mm \
    qiosurfacegraphicsbuffer.mm

HEADERS += qcocoaintegration.h \
    qcocoascreen.h \
    qcocoatheme.h \
    qcocoabackingstore.h \
    qcocoawindow.h \
    qcocoawindowmanager.h \
    qnsview.h \
    qnswindow.h \
    qnswindowdelegate.h \
    qcocoanativeinterface.h \
    qcocoaeventdispatcher.h \
    qcocoaapplicationdelegate.h \
    qcocoaapplication.h \
    qcocoansmenu.h \
    qcocoamenu.h \
    qcocoamenuitem.h \
    qcocoamenubar.h \
    qcocoamenuloader.h \
    qcocoahelpers.h \
    qmultitouch_mac_p.h \
    qcocoacursor.h \
    qcocoaclipboard.h \
    qcocoadrag.h \
    qmacclipboard.h \
    qcocoainputcontext.h \
    qcocoaservices.h \
    qcocoasystemtrayicon.h \
    qcocoaintrospection.h \
    qcocoakeymapper.h \
    qiosurfacegraphicsbuffer.h \
    qcocoamimetypes.h

qtConfig(opengl.*) {
    SOURCES += qcocoaglcontext.mm
    HEADERS += qcocoaglcontext.h
}

qtConfig(vulkan) {
    SOURCES += qcocoavulkaninstance.mm
    HEADERS += qcocoavulkaninstance.h
}

qtConfig(accessibility) {
    QT += accessibility_support-private
    SOURCES += qcocoaaccessibilityelement.mm \
        qcocoaaccessibility.mm
    HEADERS += qcocoaaccessibilityelement.h \
        qcocoaaccessibility.h
}

qtConfig(sessionmanager) {
    SOURCES += qcocoasessionmanager.cpp
    HEADERS += qcocoasessionmanager.h
}

RESOURCES += qcocoaresources.qrc

LIBS += -framework AppKit -framework CoreServices -framework Carbon -framework IOKit -framework QuartzCore -framework CoreVideo -framework Metal -framework IOSurface -lcups

DEFINES += QT_NO_FOREACH

QT += \
    core-private gui-private \
    clipboard_support-private theme_support-private \
    fontdatabase_support-private graphics_support-private

qtConfig(vulkan): QT += vulkan_support-private

CONFIG += no_app_extension_api_only

qtHaveModule(widgets) {
    QT_FOR_CONFIG += widgets

    SOURCES += qpaintengine_mac.mm
    HEADERS += qpaintengine_mac_p.h

    qtHaveModule(printsupport) {
        QT += printsupport-private
        SOURCES += \
            qprintengine_mac.mm \
            qcocoaprintersupport.mm \
            qcocoaprintdevice.mm
        HEADERS += \
            qcocoaprintersupport.h \
            qcocoaprintdevice.h \
            qprintengine_mac_p.h
    }

    qtConfig(colordialog) {
        SOURCES += qcocoacolordialoghelper.mm
        HEADERS += qcocoacolordialoghelper.h
    }

    qtConfig(filedialog) {
        SOURCES += qcocoafiledialoghelper.mm
        HEADERS += qcocoafiledialoghelper.h
    }

    qtConfig(fontdialog) {
        SOURCES += qcocoafontdialoghelper.mm
        HEADERS += qcocoafontdialoghelper.h
    }

    QT += widgets-private
}

OTHER_FILES += cocoa.json

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QCocoaIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
