TARGET = qkms

load(qt_plugin)
QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms

QT = core-private gui-private platformsupport-private opengl-private

CONFIG += link_pkgconfig qpa/genericunixfontdatabase

PKGCONFIG += libdrm egl gbm glesv2

SOURCES =   main.cpp \
            qkmsintegration.cpp \
            qkmsscreen.cpp \
            qkmscontext.cpp \
            qkmswindow.cpp \
            qkmscursor.cpp \
            qkmsdevice.cpp \
            qkmsbuffermanager.cpp \
            qkmsbackingstore.cpp
HEADERS =   qkmsintegration.h \
            qkmsscreen.h \
            qkmscontext.h \
            qkmswindow.h \
            qkmscursor.h \
            qkmsdevice.h \
            qkmsbuffermanager.h \
            qkmsbackingstore.h

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target











