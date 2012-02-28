SOURCES += syslocaleapp.cpp
DESTDIR = ./

CONFIG += console
CONFIG -= app_bundle

QT = core

# This app is testdata for tst_qlocale
target.path = $$[QT_INSTALL_TESTS]/tst_qlocale/$$TARGET
INSTALLS += target
