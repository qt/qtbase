TEMPLATE      = lib
CONFIG       += plugin
HEADERS       = theplugin.h
SOURCES       = theplugin.cpp
# Use a predictable name for the plugin, no debug extension. Just like most apps do.
#TARGET        = $$qtLibraryTarget(theplugin)
TARGET        = theplugin
DESTDIR       = ../bin
winrt:include(../winrt.pri)
QT = core

# This is testdata for the tst_qpluginloader test.
target.path = $$[QT_INSTALL_TESTS]/tst_qpluginloader/bin
INSTALLS += target
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
