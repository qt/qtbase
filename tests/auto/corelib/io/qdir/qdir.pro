CONFIG += testcase parallel_test
TARGET = tst_qdir
QT = core testlib
SOURCES = tst_qdir.cpp
RESOURCES += qdir.qrc

wince* {
  DirFiles.files = testdir testData searchdir resources entrylist types tst_qdir.cpp
  DirFiles.path = .
  DEPLOYMENT += DirFiles
  DEFINES += SRCDIR=\\\"\\\"
} else {
  DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
