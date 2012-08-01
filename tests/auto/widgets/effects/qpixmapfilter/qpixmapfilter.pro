CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qpixmapfilter

QT += widgets widgets-private testlib
QT += gui-private

SOURCES  += tst_qpixmapfilter.cpp

wince*: {
    addFiles.files = noise.png
    addFiles.path = .
    DEPLOYMENT += addFiles
}

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
