CONFIG += testcase parallel_test
TARGET = tst_qfileinfo
QT = core-private testlib
SOURCES = tst_qfileinfo.cpp
RESOURCES += qfileinfo.qrc

wince* {
    deploy.files += qfileinfo.qrc tst_qfileinfo.cpp
    res.files = resources\\file1 resources\\file1.ext1 resources\\file1.ext1.ext2
    res.path = resources
    DEPLOYMENT += deploy res
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

win32*:LIBS += -ladvapi32 -lnetapi32
