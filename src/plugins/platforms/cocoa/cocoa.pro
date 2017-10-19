TARGET = qcocoa

SOURCES += main.mm \
    qcocoaintegration.mm \
    qcocoascreen.mm \
    qcocoatheme.mm \
    qcocoabackingstore.mm \
    qcocoawindow.mm \
    qnsview.mm \
    qnswindow.mm \
    qnsviewaccessibility.mm \
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
    qcocoaaccessibilityelement.mm \
    qcocoaaccessibility.mm \
    qcocoacursor.mm \
    qcocoaclipboard.mm \
    qcocoadrag.mm \
    qmacclipboard.mm \
    qcocoasystemsettings.mm \
    qcocoainputcontext.mm \
    qcocoaservices.mm \
    qcocoasystemtrayicon.mm \
    qcocoaintrospection.mm \
    qcocoakeymapper.mm \
    qcocoamimetypes.mm \
    messages.cpp

HEADERS += qcocoaintegration.h \
    qcocoascreen.h \
    qcocoatheme.h \
    qcocoabackingstore.h \
    qcocoawindow.h \
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
    qcocoaaccessibilityelement.h \
    qcocoaaccessibility.h \
    qcocoacursor.h \
    qcocoaclipboard.h \
    qcocoadrag.h \
    qmacclipboard.h \
    qcocoasystemsettings.h \
    qcocoainputcontext.h \
    qcocoaservices.h \
    qcocoasystemtrayicon.h \
    qcocoaintrospection.h \
    qcocoakeymapper.h \
    messages.h \
    qcocoamimetypes.h

qtConfig(opengl.*) {
    SOURCES += qcocoaglcontext.mm

    HEADERS += qcocoaglcontext.h
}

RESOURCES += qcocoaresources.qrc

LIBS += -framework AppKit -framework Carbon -framework IOKit -framework QuartzCore -lcups

QT += \
    core-private gui-private \
    accessibility_support-private clipboard_support-private theme_support-private \
    fontdatabase_support-private graphics_support-private

CONFIG += no_app_extension_api_only

qtHaveModule(widgets) {
    QT_FOR_CONFIG += widgets

    SOURCES += \
        qpaintengine_mac.mm \
        qprintengine_mac.mm \
        qcocoaprintersupport.mm \
        qcocoaprintdevice.mm \

    HEADERS += \
        qpaintengine_mac_p.h \
        qprintengine_mac_p.h \
        qcocoaprintersupport.h \
        qcocoaprintdevice.h \

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

    QT += widgets-private printsupport-private
}

OTHER_FILES += cocoa.json

# Acccessibility debug support
# DEFINES += QT_COCOA_ENABLE_ACCESSIBILITY_INSPECTOR
# include ($$PWD/../../../../util/accessibilityinspector/accessibilityinspector.pri)


PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QCocoaIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
