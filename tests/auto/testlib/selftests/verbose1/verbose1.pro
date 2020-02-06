# This test just reuses the counting selftest to show how the output
# differs when the -v1 command-line switch is used.

SOURCES += ../counting/tst_counting.cpp
QT = core testlib

DEFINES += TESTLIB_VERBOSITY_ARG="-v1"

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = verbose1

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
