TARGET = qios

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QIOSIntegrationPlugin
load(qt_plugin)

QT += core-private gui-private platformsupport-private
LIBS += -framework Foundation -framework UIKit -framework QuartzCore

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
    qiosglobal.mm

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
    qiosglobal.h

#HEADERS = qiossoftwareinputhandler.h
