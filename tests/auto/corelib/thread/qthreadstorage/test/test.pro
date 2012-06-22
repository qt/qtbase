CONFIG += testcase
TARGET = ../tst_qthreadstorage
CONFIG -= app_bundle debug_and_release_target
CONFIG += console
QT = core testlib
SOURCES = ../tst_qthreadstorage.cpp

TEST_HELPER_INSTALLS = ../crashonexit/crashonexit

