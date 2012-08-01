SOURCES += tst_benchlibtickcounter.cpp
QT = core testlib-private

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target


TARGET = benchlibtickcounter
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
