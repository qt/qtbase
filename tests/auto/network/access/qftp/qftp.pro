CONFIG += testcase
TARGET = tst_qftp
SOURCES  += tst_qftp.cpp

requires(contains(QT_CONFIG,private_tests))
QT = core network network-private testlib

wince*: {
   addFiles.files = rfc3252.txt
   addFiles.path = .
   DEPLOYMENT += addFiles
   DEFINES += SRCDIR=\\\"\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

CONFIG+=insignificant_test  # QTBUG-15111: uses live qt-test-server, inherently unstable
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
