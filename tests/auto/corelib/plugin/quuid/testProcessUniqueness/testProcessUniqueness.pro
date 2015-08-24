SOURCES = main.cpp
QT = core
CONFIG += console

DESTDIR = ./

# This app is testdata for tst_quuid
target.path = $$[QT_INSTALL_TESTS]/tst_quuid/$$TARGET
INSTALLS += target
