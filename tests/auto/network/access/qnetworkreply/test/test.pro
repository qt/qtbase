CONFIG += testcase
testcase.timeout = 600 # this test is slow
CONFIG += parallel_test
CONFIG -= app_bundle debug_and_release_target
QT -= gui
SOURCES  += ../tst_qnetworkreply.cpp
TARGET = ../tst_qnetworkreply

contains(QT_CONFIG,xcb): CONFIG+=insignificant_test  # unstable, QTBUG-21102

QT = core-private network-private testlib
RESOURCES += ../qnetworkreply.qrc

TESTDATA += ../empty ../rfc3252.txt ../resource ../bigfile ../*.jpg ../certs \
            ../index.html ../smb-file.txt

win32:CONFIG += insignificant_test # QTBUG-24226

TEST_HELPER_INSTALLS = ../echo/echo
