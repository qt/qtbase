SOURCES = main.cpp
CONFIG -= qt app_bundle
CONFIG += console
DESTDIR = ./

TARGET = two space s

# This app is testdata for tst_qprocess
target.path = $$[QT_INSTALL_TESTS]/tst_qprocess/testProcessSpacesArgs
INSTALLS += target
