TARGET = qglgraphicssystem
include(../../qpluginbase.pri)

QT += opengl

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/graphicssystems

SOURCES = main.cpp

target.path += $$[QT_INSTALL_PLUGINS]/graphicssystems
INSTALLS += target

symbian: TARGET.UID3 = 0x2002131B
