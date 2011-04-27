TARGET = qeglnullws
include(../../qpluginbase.pri)

CONFIG += warn_on
QT += opengl

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/gfxdrivers

target.path = $$[QT_INSTALL_PLUGINS]/gfxdrivers
INSTALLS += target

HEADERS = eglnullwsscreen.h \
          eglnullwsscreenplugin.h \
          eglnullwswindowsurface.h

SOURCES = eglnullwsscreen.cpp \
          eglnullwsscreenplugin.cpp \
          eglnullwswindowsurface.cpp
