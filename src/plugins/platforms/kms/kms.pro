TARGET = qkms
load(qt_plugin)

QT += core-private gui-private platformsupport-private opengl-private
DESTDIR = $$QT.gui.plugins/platforms

DEFINES += MESA_EGL_NO_X11_HEADERS

CONFIG += link_pkgconfig egl qpa/genericunixfontdatabase

PKGCONFIG += libdrm libudev egl gbm glesv2

SOURCES =   main.cpp \
            qkmsintegration.cpp \
            qkmsscreen.cpp \
            qkmscontext.cpp \
            qkmswindow.cpp \
            qkmscursor.cpp \
            qkmsdevice.cpp \
            qkmsbuffermanager.cpp \
            qkmsbackingstore.cpp \
            qkmsnativeinterface.cpp \
            qkmsudevlistener.cpp \
            qkmsudevhandler.cpp \
            qkmsudevdrmhandler.cpp
HEADERS =   qkmsintegration.h \
            qkmsscreen.h \
            qkmscontext.h \
            qkmswindow.h \
            qkmscursor.h \
            qkmsdevice.h \
            qkmsbuffermanager.h \
            qkmsbackingstore.h \
            qkmsnativeinterface.h \
            qkmsudevlistener.h \
            qkmsudevhandler.h \
            qkmsudevdrmhandler.h

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target

OTHER_FILES += \
    kms.json
