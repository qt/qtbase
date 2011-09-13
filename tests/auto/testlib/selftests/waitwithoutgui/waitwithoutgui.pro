load(qttest_p4)

# this is not a real testcase ('make check' should not run it)
CONFIG -= testcase

QT -= gui
SOURCES += tst_waitwithoutgui.cpp

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target


TARGET = waitwithoutgui
