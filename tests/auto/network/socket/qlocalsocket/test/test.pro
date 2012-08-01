CONFIG += testcase

DEFINES += QLOCALSERVER_DEBUG
DEFINES += QLOCALSOCKET_DEBUG

wince* {
    DEFINES += QT_LOCALSOCKET_TCP
    DEFINES += SRCDIR=\\\"../\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD/../\\\"
}

QT = core network testlib

SOURCES += ../tst_qlocalsocket.cpp

TARGET = tst_qlocalsocket
CONFIG(debug_and_release) {
  CONFIG(debug, debug|release) {
    DESTDIR = ../debug
  } else {
    DESTDIR = ../release
  }
} else {
  DESTDIR = ..
}

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
