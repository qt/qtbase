CONFIG += testcase
TARGET = tst_qiodevice
SOURCES  += tst_qiodevice.cpp

QT = core network testlib

wince*: {
   addFiles.files = tst_qiodevice.cpp
   addFiles.path = .
   DEPLOYMENT += addFiles
   DEFINES += SRCDIR=\\\"\\\"
   !wince50standard-x86-msvc2005: DEFINES += WINCE_EMULATOR_TEST=1
} else {
   DEFINES += SRCDIR=\\\"$$PWD/\\\"
}
MOC_DIR=tmp


