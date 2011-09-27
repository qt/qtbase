load(qttest_p4)
SOURCES   += tst_qdir.cpp
RESOURCES += qdir.qrc
QT        = core

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
