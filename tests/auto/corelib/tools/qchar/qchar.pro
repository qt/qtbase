load(qttest_p4)
SOURCES  += tst_qchar.cpp

QT = core core-private

wince*: {
    deploy.files += NormalizationTest.txt
    DEPLOYMENT += deploy
}

DEFINES += SRCDIR=\\\"$$PWD/\\\"

CONFIG += parallel_test
