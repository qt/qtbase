CONFIG += testcase
TARGET = tst_qfontdatabase
SOURCES  += tst_qfontdatabase.cpp
DEFINES += SRCDIR=\\\"$$PWD\\\"
QT += testlib

wince* {
    additionalFiles.files = FreeMono.ttf
    additionalFiles.path = .
    DEPLOYMENT += additionalFiles
}

mac: CONFIG += insignificant_test # QTBUG-23062
win32:CONFIG += insignificant_test # QTBUG-24193
