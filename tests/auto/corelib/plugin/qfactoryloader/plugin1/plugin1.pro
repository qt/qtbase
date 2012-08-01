TEMPLATE      = lib
CONFIG       += plugin
HEADERS       = plugin1.h
SOURCES       = plugin1.cpp
TARGET        = $$qtLibraryTarget(plugin1)
DESTDIR       = ../bin

# This is testdata for the tst_qpluginloader test.
target.path = $$[QT_INSTALL_TESTS]/tst_qfactoryloader/bin
INSTALLS += target
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
