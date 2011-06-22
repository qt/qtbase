TARGET = qcocoa
load(qt_plugin)
DESTDIR = $$QT.gui.plugins/platforms

OBJECTIVE_SOURCES += main.mm \
    qcocoaintegration.mm \
    qcocoabackingstore.mm \
    qcocoawindow.mm \
    qnsview.mm \
    qcocoaautoreleasepool.mm \
    qnswindowdelegate.mm \
    qcocoaglcontext.mm \
    qcocoanativeinterface.mm \
    qcocoaeventdispatcher.mm

HEADERS += qcocoaintegration.h \
    qcocoabackingstore.h \
    qcocoawindow.h \
    qnsview.h \
    qcocoaautoreleasepool.h \
    qnswindowdelegate.h \
    qcocoaglcontext.h \
    qcocoanativeinterface.h \
    qcocoaeventdispatcher.h

#add libz for freetype.
LIBS += -lz -framework Cocoa

QT += core-private gui-private platformsupport-private

CONFIG += qpa/basicunixfontdatabase
target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target
