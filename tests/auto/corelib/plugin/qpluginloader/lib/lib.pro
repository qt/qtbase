TEMPLATE = lib
CONFIG += dll
CONFIG -= staticlib
SOURCES		= mylib.c
TARGET = tst_qpluginloaderlib
DESTDIR = ../bin
winrt:include(../winrt.pri)
QT = core

win32-msvc: DEFINES += WIN32_MSVC

# This is testdata for the tst_qpluginloader test.
target.path = $$[QT_INSTALL_TESTS]/tst_qpluginloader/bin
INSTALLS += target
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
