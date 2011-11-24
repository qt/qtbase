SOURCES = main.cpp
CONFIG -= qt app_bundle
CONFIG += console
DESTDIR = ./

# This app is testdata for tst_qproces
target.path = $$[QT_INSTALL_TESTS]/tst_qprocess/testProcessSpacesArgs
INSTALLS += target
