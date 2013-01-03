TEMPLATE      = lib
CONFIG       += plugin
HEADERS       = almostplugin.h
SOURCES       = almostplugin.cpp
TARGET        = almostplugin
DESTDIR       = ../bin
QT = core
*-g++*:QMAKE_LFLAGS -= -Wl,--no-undefined

# This is testdata for the tst_qpluginloader test.
target.path = $$[QT_INSTALL_TESTS]/tst_qpluginloader/bin
INSTALLS += target
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
