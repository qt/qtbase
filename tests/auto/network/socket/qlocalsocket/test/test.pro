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

wince* {
    additionalFiles.files = ../lackey/lackey.exe
    additionalFiles.path = lackey
}

wince* {
    scriptFiles.files = ../lackey/scripts/*.js
    scriptFiles.path = lackey/scripts
    DEPLOYMENT += additionalFiles scriptFiles
    QT += script    # for easy deployment of QtScript
    
    requires(contains(QT_CONFIG,script))
}

