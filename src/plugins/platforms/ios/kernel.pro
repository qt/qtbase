TARGET = qios

# QTBUG-42937: Work around linker errors caused by circular
# dependencies between the iOS platform plugin and the user
# application's main() when the plugin is a shared library.
qtConfig(shared): CONFIG += static

QT += \
    core-private gui-private \
    clipboard_support-private fontdatabase_support-private graphics_support-private

LIBS += -framework Foundation -framework UIKit -framework QuartzCore -framework AudioToolbox

OBJECTIVE_SOURCES = \
    plugin.mm \
    qiosintegration.mm \
    qioseventdispatcher.mm \
    qioswindow.mm \
    qiosscreen.mm \
    qiosbackingstore.mm \
    qiosapplicationdelegate.mm \
    qiosapplicationstate.mm \
    qiosviewcontroller.mm \
    qioscontext.mm \
    qiosinputcontext.mm \
    qiostheme.mm \
    qiosglobal.mm \
    qiosservices.mm \
    quiview.mm \
    quiaccessibilityelement.mm \
    qiosplatformaccessibility.mm \
    qiostextresponder.mm

HEADERS = \
    qiosintegration.h \
    qioseventdispatcher.h \
    qioswindow.h \
    qiosscreen.h \
    qiosbackingstore.h \
    qiosapplicationdelegate.h \
    qiosapplicationstate.h \
    qiosviewcontroller.h \
    qioscontext.h \
    qiosinputcontext.h \
    qiostheme.h \
    qiosglobal.h \
    qiosservices.h \
    quiview.h \
    quiaccessibilityelement.h \
    qiosplatformaccessibility.h \
    qiostextresponder.h

!tvos {
    LIBS += -framework AssetsLibrary
    OBJECTIVE_SOURCES += \
        qiosclipboard.mm \
        qiosmenu.mm \
        qiosfiledialog.mm \
        qiosmessagedialog.mm \
        qiostextinputoverlay.mm \
        qiosdocumentpickercontroller.mm
    HEADERS += \
        qiosclipboard.h \
        qiosmenu.h \
        qiosfiledialog.h \
        qiosmessagedialog.h \
        qiostextinputoverlay.h \
        qiosdocumentpickercontroller.h
}

OTHER_FILES = \
    quiview_textinput.mm \
    quiview_accessibility.mm

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QIOSIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
