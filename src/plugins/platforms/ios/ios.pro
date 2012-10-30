TARGET = qios
include(../../qpluginbase.pri)
QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms

QT += opengl
QT += core-private gui-private platformsupport-private opengl-private widgets-private
LIBS += -framework Cocoa -framework UIKit

OBJECTIVE_SOURCES = main.mm \
    qiosintegration.mm \
    qioswindow.mm \
    qiosscreen.mm \
    qioseventdispatcher.mm \
    qiosbackingstore.mm

OBJECTIVE_HEADERS = qiosintegration.h \
    qioswindow.h \
    qiosscreen.h \
    qioseventdispatcher.h \
    qiosbackingstore.h

#HEADERS = qiossoftwareinputhandler.h

#include(../fontdatabases/coretext/coretext.pri)

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target
