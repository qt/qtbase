TARGET = qeglfs
TEMPLATE = lib
CONFIG += plugin

QT += opengl

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms

#DEFINES += QEGL_EXTRA_DEBUG

#DEFINES += Q_OPENKODE

SOURCES =   main.cpp \
            qeglfsintegration.cpp \
            ../eglconvenience/qeglconvenience.cpp \
            ../eglconvenience/qeglplatformcontext.cpp \
            qeglfswindow.cpp \
            qeglfswindowsurface.cpp \
            qeglfsscreen.cpp

HEADERS =   qeglfsintegration.h \
            ../eglconvenience/qeglconvenience.h \
            ../eglconvenience/qeglplatformcontext.h \
            qeglfswindow.h \
            qeglfswindowsurface.h \
            qeglfsscreen.h

include(../fontdatabases/genericunix/genericunix.pri)

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target
