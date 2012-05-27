TARGET = qminimalegl
load(qt_plugin)

QT += core-private gui-private platformsupport-private

DESTDIR = $$QT.gui.plugins/platforms

#DEFINES += QEGL_EXTRA_DEBUG

#DEFINES += Q_OPENKODE

#Avoid X11 header collision
DEFINES += MESA_EGL_NO_X11_HEADERS

SOURCES =   main.cpp \
            qminimaleglintegration.cpp \
            qminimaleglwindow.cpp \
            qminimaleglbackingstore.cpp \
            qminimaleglscreen.cpp

HEADERS =   qminimaleglintegration.h \
            qminimaleglwindow.h \
            qminimaleglbackingstore.h \
            qminimaleglscreen.h

CONFIG += egl qpa/genericunixfontdatabase

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target

OTHER_FILES += \
    minimalegl.json
