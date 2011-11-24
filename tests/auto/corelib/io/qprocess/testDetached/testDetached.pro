SOURCES = main.cpp
QT = core
CONFIG += console
CONFIG -= app_bundle
INSTALLS =
DESTDIR = ./

# This app is testdata for tst_qprocess
target.path = $$[QT_INSTALL_TESTS]/tst_qprocess/$$TARGET
INSTALLS += target
