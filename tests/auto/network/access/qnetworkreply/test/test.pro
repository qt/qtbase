CONFIG += testcase
testcase.timeout = 600 # this test is slow
CONFIG -= debug_and_release_target
INCLUDEPATH += ../../../../../shared/
HEADERS += ../../../../../shared/emulationdetector.h
SOURCES  += ../tst_qnetworkreply.cpp
TARGET = ../tst_qnetworkreply

QT = core-private network-private testlib
QT_FOR_CONFIG += gui-private
RESOURCES += ../qnetworkreply.qrc

TESTDATA += ../empty ../rfc3252.txt ../resource ../bigfile ../*.jpg ../certs \
            ../index.html ../smb-file.txt

!android:!winrt: TEST_HELPER_INSTALLS = ../echo/echo
