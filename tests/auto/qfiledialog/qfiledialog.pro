############################################################
# Project file for autotest for file qfiledialog.h
############################################################

load(qttest_p4)
QT += widgets widgets-private
QT += core-private gui-private
SOURCES += tst_qfiledialog.cpp

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

