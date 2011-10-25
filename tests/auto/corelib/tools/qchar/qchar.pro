CONFIG += testcase
TARGET = tst_qchar
SOURCES  += tst_qchar.cpp

QT = core core-private testlib

wince*: {
    deploy.files += NormalizationTest.txt
    DEPLOYMENT += deploy
}

DEFINES += SRCDIR=\\\"$$PWD/\\\"

CONFIG += parallel_test
