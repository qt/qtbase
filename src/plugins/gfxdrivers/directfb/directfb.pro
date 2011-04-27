TARGET = qdirectfbscreen
include(../../qpluginbase.pri)
include($$QT_SOURCE_TREE/src/gui/embedded/directfb.pri)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/gfxdrivers

target.path = $$[QT_INSTALL_PLUGINS]/gfxdrivers
INSTALLS += target

SOURCES += qdirectfbscreenplugin.cpp

QMAKE_CXXFLAGS += $$QT_CFLAGS_DIRECTFB
LIBS += $$QT_LIBS_DIRECTFB
DEFINES += $$QT_DEFINES_DIRECTFB
contains(gfx-plugins, directfb):DEFINES += QT_QWS_DIRECTFB
