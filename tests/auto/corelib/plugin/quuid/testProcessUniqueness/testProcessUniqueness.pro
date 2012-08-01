SOURCES = main.cpp
CONFIG += console

DESTDIR = ./

# This app is testdata for tst_quuid
target.path = $$[QT_INSTALL_TESTS]/tst_quuid/$$TARGET
INSTALLS += target
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
