SOURCES += main.cpp
QT = core
CONFIG -= app_bundle debug_and_release_target
CONFIG += console

# This app is testdata for tst_qfile
target.path = $$[QT_INSTALL_TESTS]/tst_qfile/$$TARGET
INSTALLS += target
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
