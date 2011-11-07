CONFIG += testcase
TARGET = ../tst_qlibrary
QT = core testlib
SOURCES = ../tst_qlibrary.cpp

win32 {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qlibrary
    } else {
        TARGET = ../../release/tst_qlibrary
    }
}

wince* {
    addFiles.files = ../*.dll ../*.dl2 ../mylib_noextension
    addFiles.path = .
    DEPLOYMENT += addFiles
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/../\\\"
}
