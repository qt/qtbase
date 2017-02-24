TEMPLATE      = lib
QT            = core
CONFIG       += plugin
HEADERS       = plugin1.h
SOURCES       = plugin1.cpp
TARGET        = $$qtLibraryTarget(plugin1)
DESTDIR       = ../bin
winrt:include(../winrt.pri)

!qtConfig(library): DEFINES += QT_STATICPLUGIN

# This is testdata for the tst_qpluginloader test.
target.path = $$[QT_INSTALL_TESTS]/tst_qfactoryloader/bin
INSTALLS += target
