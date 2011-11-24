SOURCES = main.cpp
CONFIG -= qt
CONFIG += console
DESTDIR = ./

mac {
  CONFIG -= app_bundle
}

# This app is testdata for tst_qprocess
target.path = $$[QT_INSTALL_TESTS]/tst_qprocess/$$TARGET
INSTALLS += target
