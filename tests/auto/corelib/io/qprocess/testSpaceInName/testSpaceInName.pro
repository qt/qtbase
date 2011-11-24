SOURCES = main.cpp
CONFIG -= qt
CONFIG += console
DESTDIR = "../test Space In Name"

mac {
  CONFIG -= app_bundle
}

# This app is testdata for tst_qprocess
target.path = "$$[QT_INSTALL_TESTS]/tst_qprocess/test Space In Name"
INSTALLS += target
