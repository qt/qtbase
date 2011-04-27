load(qttest_p4)
SOURCES += tst_benchlibcallgrind.cpp
QT = core

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target


TARGET = benchlibcallgrind
