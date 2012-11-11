TARGET = qios

load(qt_plugin)

QT += core-private gui-private platformsupport-private
LIBS += -framework UIKit -framework QuartzCore

OBJECTIVE_SOURCES = \
    plugin.mm \
    qiosintegration.mm \
    qioswindow.mm \
    qiosscreen.mm \
    qioseventdispatcher.mm \
    qiosbackingstore.mm \
    qiosapplicationdelegate.mm \
    qioscontext.mm

HEADERS = \
    qiosintegration.h \
    qioswindow.h \
    qiosscreen.h \
    qioseventdispatcher.h \
    qiosbackingstore.h \
    qiosapplicationdelegate.h \
    qioscontext.h

#HEADERS = qiossoftwareinputhandler.h

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target
