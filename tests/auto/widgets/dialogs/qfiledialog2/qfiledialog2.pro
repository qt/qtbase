CONFIG += testcase
TARGET = tst_qfiledialog2

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES += tst_qfiledialog2.cpp

wince* {
    addFiles.files = *.cpp
    addFiles.path = .
    filesInDir.files = *.pro
    filesInDir.path = someDir
    DEPLOYMENT += addFiles filesInDir
}

wince* {
    DEFINES += SRCDIR=\\\"./\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
