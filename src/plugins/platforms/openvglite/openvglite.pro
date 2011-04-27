TARGET = qvglitegraphicssystem
include(../../qpluginbase.pri)

QT += openvg

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/graphicssystems

SOURCES = main.cpp qgraphicssystem_vglite.cpp qwindowsurface_vglite.cpp
HEADERS = qgraphicssystem_vglite.h qwindowsurface_vglite.h

target.path += $$[QT_INSTALL_PLUGINS]/graphicssystems
INSTALLS += target
