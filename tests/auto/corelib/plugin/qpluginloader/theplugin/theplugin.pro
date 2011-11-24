TEMPLATE      = lib
CONFIG       += plugin
HEADERS       = theplugin.h
SOURCES       = theplugin.cpp
TARGET        = $$qtLibraryTarget(theplugin)
DESTDIR       = ../bin

# This is testdata for the tst_qpluginloader test.
target.path = $$[QT_INSTALL_TESTS]/tst_qpluginloader/bin
INSTALLS += target
