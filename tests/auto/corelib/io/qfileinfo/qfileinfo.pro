load(qttest_p4)
SOURCES  += tst_qfileinfo.cpp

QT = core-private


RESOURCES      += qfileinfo.qrc

wince* {
    deploy.files += qfileinfo.qrc tst_qfileinfo.cpp
    res.files = resources\\file1 resources\\file1.ext1 resources\\file1.ext1.ext2
    res.path = resources
    DEPLOYMENT += deploy res
}

win32*:LIBS += -ladvapi32 -lnetapi32

# support for running test from shadow build directory
wince* {
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

CONFIG += parallel_test
