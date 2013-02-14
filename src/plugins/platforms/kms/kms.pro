TARGET = qkms

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QKmsIntegrationPlugin
load(qt_plugin)

QT += core-private gui-private platformsupport-private
qtHaveModule(opengl):QT += opengl-private

DEFINES += MESA_EGL_NO_X11_HEADERS __GBM__

CONFIG += link_pkgconfig egl qpa/genericunixfontdatabase

PKGCONFIG += libdrm libudev egl gbm glesv2

SOURCES =   main.cpp \
            qkmsintegration.cpp \
            qkmsscreen.cpp \
            qkmscontext.cpp \
            qkmswindow.cpp \
            qkmscursor.cpp \
            qkmsdevice.cpp \
            qkmsbackingstore.cpp \
            qkmsnativeinterface.cpp \
            qkmsudevlistener.cpp \
            qkmsudevhandler.cpp \
            qkmsudevdrmhandler.cpp \
            qkmsvthandler.cpp
HEADERS =   qkmsintegration.h \
            qkmsscreen.h \
            qkmscontext.h \
            qkmswindow.h \
            qkmscursor.h \
            qkmsdevice.h \
            qkmsbackingstore.h \
            qkmsnativeinterface.h \
            qkmsudevlistener.h \
            qkmsudevhandler.h \
            qkmsudevdrmhandler.h \
            qkmsvthandler.h

OTHER_FILES += \
    kms.json
