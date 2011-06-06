TARGET = qcocoa
load(qpa/plugin)
DESTDIR = $$QT.gui.plugins/platforms

OBJECTIVE_SOURCES = main.mm \
    qcocoaintegration.mm \
    qcocoawindowsurface.mm \
    qcocoawindow.mm \
    qnsview.mm \
    qcocoaeventloopintegration.mm \
    qcocoaautoreleasepool.mm \
    qnswindowdelegate.mm \
    qcocoaglcontext.mm \
    qcocoanativeinterface.mm


OBJECTIVE_HEADERS = qcocoaintegration.h \
    qcocoawindowsurface.h \
    qcocoawindow.h \
    qnsview.h \
    qcocoaeventloopintegration.h \
    qcocoaautoreleasepool.h \
    qnswindowdelegate.h \
    qcocoaglcontext.h \
    qcocoanativeinterface.h

DEFINES += QT_BUILD_COCOA_LIB

#add libz for freetype.
LIBS += -lz
LIBS += -framework cocoa

QT += core-private gui-private

load(qpa/fontdatabases/basicunix)
target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target
