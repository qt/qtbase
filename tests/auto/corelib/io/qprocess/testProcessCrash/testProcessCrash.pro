SOURCES = main.cpp
CONFIG += console
CONFIG -= qt app_bundle

DESTDIR = ./

# This app is testdata for tst_qprocess
target.path = $$[QT_INSTALL_TESTS]/tst_qprocess/$$TARGET
INSTALLS += target
