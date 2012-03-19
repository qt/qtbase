TARGET = qeglfs
load(qt_plugin)

QT += core-private gui-private platformsupport-private

!contains(QT_CONFIG, no-widgets) {
    QT += opengl opengl-private widgets-private
}

DESTDIR = $$QT.gui.plugins/platforms

#DEFINES += QEGL_EXTRA_DEBUG

#DEFINES += Q_OPENKODE

#Avoid X11 header collision
DEFINES += MESA_EGL_NO_X11_HEADERS

SOURCES =   main.cpp \
            qeglfsintegration.cpp \
            qeglfswindow.cpp \
            qeglfsbackingstore.cpp \
            qeglfsscreen.cpp

HEADERS =   qeglfsintegration.h \
            qeglfswindow.h \
            qeglfsbackingstore.h \
            qeglfsscreen.h

CONFIG += egl qpa/genericunixfontdatabase

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target

OTHER_FILES += \
    eglfs.json
