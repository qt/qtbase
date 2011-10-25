CONFIG += testcase
TARGET = tst_qdir
SOURCES   += tst_qdir.cpp
RESOURCES += qdir.qrc
QT        = core testlib

wince* {
  DirFiles.files = testdir testData searchdir resources entrylist types tst_qdir.cpp
  DirFiles.path = .
  DEPLOYMENT += DirFiles
}

wince* {
  DEFINES += SRCDIR=\\\"\\\"
} else {
  DEFINES += SRCDIR=\\\"$$PWD/\\\"
}

CONFIG += parallel_test
