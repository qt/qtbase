TARGET = qios

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QIOSIntegrationPlugin
load(qt_plugin)

QT += core-private gui-private platformsupport-private
LIBS += -framework Foundation -framework UIKit -framework QuartzCore

OBJECTIVE_SOURCES = \
    plugin.mm \
    qiosmain_wrapper.mm \
    qiosmain_dummy.mm \
    qiosintegration.mm \
    qioswindow.mm \
    qiosscreen.mm \
    qiosbackingstore.mm \
    qiosapplicationdelegate.mm \
    qiosapplicationstate.mm \
    qiosviewcontroller.mm \
    qioscontext.mm \
    qiosinputcontext.mm \
    qiostheme.mm \
    qiosglobal.mm

HEADERS = \
    qiosintegration.h \
    qioswindow.h \
    qiosscreen.h \
    qiosbackingstore.h \
    qiosapplicationdelegate.h \
    qiosapplicationstate.h \
    qiosviewcontroller.h \
    qioscontext.h \
    qiosinputcontext.h \
    qiostheme.h \
    qiosglobal.h

#HEADERS = qiossoftwareinputhandler.h
