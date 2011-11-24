SOURCES = main.cpp
CONFIG += console
CONFIG -= app_bundle
QT = core
DESTDIR = ./

# This app is testdata for tst_qprocess
target.path = $$[QT_INSTALL_TESTS]/tst_qprocess/$$TARGET
INSTALLS += target
