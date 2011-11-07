CONFIG += testcase
TARGET = tst_qdatastream
QT = gui widgets testlib
SOURCES = tst_qdatastream.cpp

wince* {
    addFiles.files = datastream.q42
    addFiles.path = .
    DEPLOYMENT += addFiles
    DEFINES += SRCDIR=\\\"\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
