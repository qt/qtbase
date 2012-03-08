# This test just reuses the counting selftest to show how the output
# differs when the -v2 command-line switch is used.

SOURCES += ../counting/tst_counting.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = verbose2
