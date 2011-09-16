load(qttest_p4)
QT = core

# this is not a real testcase ('make check' should not run it)
CONFIG -= testcase

SOURCES  += tst_xunit.cpp

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = xunit

