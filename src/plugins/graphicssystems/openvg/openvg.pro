TARGET = qvggraphicssystem
include(../../qpluginbase.pri)

QT += openvg

DESTDIR = $$QT.gui.plugins/graphicssystems

SOURCES = main.cpp qgraphicssystem_vg.cpp
HEADERS = qgraphicssystem_vg_p.h

target.path += $$[QT_INSTALL_PLUGINS]/graphicssystems
INSTALLS += target

symbian: TARGET.UID3 = 0x2001E62C
