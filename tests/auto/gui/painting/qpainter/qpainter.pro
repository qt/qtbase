CONFIG += testcase
TARGET = tst_qpainter

QT += widgets widgets-private printsupport testlib

SOURCES  += tst_qpainter.cpp
wince* {
    addFiles.files = drawEllipse drawLine_rop_bitmap drawPixmap_rop drawPixmap_rop_bitmap task217400.png
    addFiles.path = .
    DEPLOYMENT += addFiles
    DEFINES += SRCDIR=\\\".\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

mac*:CONFIG+=insignificant_test
