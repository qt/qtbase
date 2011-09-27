load(qttest_p4)

include(../src/src.pri)
QT -= gui

DEFINES	+= QSHAREDMEMORY_DEBUG
DEFINES	+= QSYSTEMSEMAPHORE_DEBUG

SOURCES += ../tst_qsharedmemory.cpp
TARGET = ../tst_qsharedmemory

!wince*:win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_qsharedmemory
} else {
    TARGET = ../../release/tst_qsharedmemory
  }
}

wince*:{
requires(contains(QT_CONFIG,script))
QT += gui script
addFiles.files = $$OUT_PWD/../../lackey/lackey.exe ../../lackey/scripts
addFiles.path = .
DEPLOYMENT += addFiles
DEFINES += SRCDIR=\\\".\\\"
} else {
DEFINES += SRCDIR=\\\"$$PWD/../\\\"
}
