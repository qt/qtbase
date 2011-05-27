TARGET = qtracegraphicssystem
load(qt_plugin)

QT += core-private gui-private network

DESTDIR = $$QT.gui.plugins/graphicssystems
symbian:TARGET.UID3 = 0x2002130E

SOURCES = main.cpp qgraphicssystem_trace.cpp

target.path += $$[QT_INSTALL_PLUGINS]/graphicssystems
INSTALLS += target
INCLUDEPATH += ../../../3rdparty/harfbuzz/src
