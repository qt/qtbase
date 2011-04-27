load(qttest_p4)
SOURCES  += tst_qchar.cpp

QT = core

wince*|symbian: {
deploy.files += NormalizationTest.txt
DEPLOYMENT += deploy
}

symbian: {
    DEFINES += SRCDIR=""
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
CONFIG += parallel_test
