CONFIG += testcase
testcase.timeout = 600 # this test is slow
CONFIG -= app_bundle debug_and_release_target
SOURCES  += ../tst_qnetworkreply.cpp
TARGET = ../tst_qnetworkreply

QT = core-private network-private testlib
RESOURCES += ../qnetworkreply.qrc

TESTDATA += ../empty ../rfc3252.txt ../resource ../bigfile ../*.jpg ../certs \
            ../index.html ../smb-file.txt

contains(QT_CONFIG,xcb): CONFIG+=insignificant_test  # unstable, QTBUG-21102
win32:CONFIG += insignificant_test # QTBUG-24226

TEST_HELPER_INSTALLS = ../echo/echo
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
