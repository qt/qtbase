CONFIG += testcase
TARGET = ../tst_qthreadstorage
CONFIG -= app_bundle
CONFIG += console
QT = core testlib
SOURCES = ../tst_qthreadstorage.cpp

load(testcase) # for installTestHelperApp()
installTestHelperApp("../crashonexit/crashonexit",crashonexit,crashonexit)

