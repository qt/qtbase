CONFIG += testcase
TARGET = tst_qpixmapfilter

QT += widgets widgets-private testlib
QT += gui-private

SOURCES  += tst_qpixmapfilter.cpp

wince {
    addFiles.files = noise.png
    addFiles.path = .
    DEPLOYMENT += addFiles
}

