SOURCES += tst_benchlibtickcounter.cpp
QT = core testlib-private

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target


TARGET = benchlibtickcounter

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
