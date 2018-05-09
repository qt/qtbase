CONFIG += testcase
TARGET = ../tst_qthreadstorage
CONFIG -= debug_and_release_target
CONFIG += console
QT = core testlib
SOURCES = ../tst_qthreadstorage.cpp

!android:!winrt: TEST_HELPER_INSTALLS = ../crashonexit/crashonexit

