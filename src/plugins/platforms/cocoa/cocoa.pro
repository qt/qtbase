TARGET = qcocoa
load(qt_plugin)
DESTDIR = $$QT.gui.plugins/platforms

OBJECTIVE_SOURCES = main.mm \
    qcocoaintegration.mm \
    qcocoawindowsurface.mm \
    qcocoawindow.mm \
    qnsview.mm \
    qcocoaautoreleasepool.mm \
    qnswindowdelegate.mm \
    qcocoaglcontext.mm \
    qcocoanativeinterface.mm


OBJECTIVE_HEADERS = qcocoaintegration.h \
    qcocoawindowsurface.h \
    qcocoawindow.h \
    qnsview.h \
    qcocoaautoreleasepool.h \
    qnswindowdelegate.h \
    qcocoaglcontext.h \
    qcocoanativeinterface.h

DEFINES += QT_BUILD_COCOA_LIB

#add libz for freetype.
LIBS += -lz
LIBS += -framework cocoa

QT += core-private gui-private platformsupport-private
LIBS += -lQtPlatformSupport

CONFIG += qpa/basicunixfontdatabase
target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target
