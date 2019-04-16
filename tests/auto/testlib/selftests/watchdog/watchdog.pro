SOURCES += tst_watchdog.cpp
QT = core testlib

darwin: CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = watchdog

# The test deliberately times out; so tell it to do so quickly
checkenv.name = QTEST_FUNCTION_TIMEOUT
checkenv.value = 100
QT_TOOL_ENV += checkenv

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
