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
    qiosurfacegraphicsbuffer.mm \
    qcocoacolordialoghelper.mm \
    qcocoafiledialoghelper.mm \
    qcocoafontdialoghelper.mm

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
    qcocoamimetypes.h \
    qcocoacolordialoghelper.h \
    qcocoafiledialoghelper.h \
    qcocoafontdialoghelper.h

qtConfig(opengl.*) {
    SOURCES += qcocoaglcontext.mm
    HEADERS += qcocoaglcontext.h
}

qtConfig(vulkan) {
    SOURCES += qcocoavulkaninstance.mm
    HEADERS += qcocoavulkaninstance.h
}

qtConfig(accessibility) {
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

LIBS += -framework AppKit -framework CoreServices -framework Carbon -framework IOKit -framework QuartzCore -framework CoreVideo -framework Metal -framework IOSurface

DEFINES += QT_NO_FOREACH

QT += core-private gui-private

CONFIG += no_app_extension_api_only

OTHER_FILES += cocoa.json

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QCocoaIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
