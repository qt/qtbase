CONFIG += testcase
TARGET = tst_qabstractnetworkcache
QT += network testlib
QT -= gui
SOURCES  += tst_qabstractnetworkcache.cpp

wince* {
   testFiles.files = tests
   testFiles.path = .
   DEPLOYMENT += testFiles
}

CONFIG += insignificant_test  # QTBUG-20686; note, assumed unstable on all platforms
