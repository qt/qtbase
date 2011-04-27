TARGET = qvggraphicssystem
include(../../qpluginbase.pri)

QT += openvg

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/graphicssystems

SOURCES = main.cpp qgraphicssystem_vg.cpp
HEADERS = qgraphicssystem_vg_p.h

target.path += $$[QT_INSTALL_PLUGINS]/graphicssystems
INSTALLS += target

symbian: TARGET.UID3 = 0x2001E62C
