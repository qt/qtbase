TEMPLATE      = lib
CONFIG       += plugin
HEADERS       = almostplugin.h
SOURCES       = almostplugin.cpp
TARGET        = almostplugin
DESTDIR       = ../bin
*-g++*:QMAKE_LFLAGS -= -Wl,--no-undefined

# This is testdata for the tst_qpluginloader test.
target.path = $$[QT_INSTALL_TESTS]/tst_qpluginloader/bin
INSTALLS += target
