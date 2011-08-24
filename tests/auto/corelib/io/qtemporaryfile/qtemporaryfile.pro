load(qttest_p4)
SOURCES       += tst_qtemporaryfile.cpp
QT = core


symbian {
    testData.files = tst_qtemporaryfile.cpp
    testData.path = .
    DEPLOYMENT += testData
}else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

CONFIG += parallel_test
