SOURCES += tst_datatable.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = datatable
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
