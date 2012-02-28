SOURCES += crashOnExit.cpp
DESTDIR = ./
QT = core
CONFIG -= app_bundle
CONFIG += console

# This app is testdata for tst_qthreadstorage
target.path = $$[QT_INSTALL_TESTS]/tst_qthreadstorage/$$TARGET
INSTALLS += target
