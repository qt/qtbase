CONFIG += testcase
CONFIG += parallel_test
CONFIG -= app_bundle debug_and_release_target
QT = core testlib network
embedded: QT += gui
SOURCES = ../tst_qprocess.cpp

TARGET = ../tst_qprocess

win32:TESTDATA += ../testBatFiles/*

include(../qprocess.pri)

mac:CONFIG += insignificant_test # QTBUG-25895 - sometimes hangs

for(file, SUBPROGRAMS): TEST_HELPER_INSTALLS += "../$${file}/$${file}"

TEST_HELPER_INSTALLS += \
    ../testProcessSpacesArgs/nospace \
    "../testProcessSpacesArgs/one space" \
    "../testProcessSpacesArgs/two space s" \
    "../test Space In Name/testSpaceInName"
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
