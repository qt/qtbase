SOURCES = main.cpp
CONFIG -= qt app_bundle
CONFIG += console

win32:!win32-g++*:!equals(TEMPLATE_PREFIX, "vc"):QMAKE_CXXFLAGS += /GS-
DESTDIR = ./

# This app is testdata for tst_qprocess
target.path = $$[QT_INSTALL_TESTS]/tst_qprocess/$$TARGET
INSTALLS += target
