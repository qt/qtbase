load(qttest_p4)
SOURCES  += ../tst_qudpsocket.cpp
QT = core network

MOC_DIR=tmp

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = ../debug
} else {
    DESTDIR = ../release
  }
} else {
    DESTDIR = ../
}

wince*|symbian: {
    addApp.files = ../clientserver/clientserver.exe
    addApp.path = clientserver
    DEPLOYMENT += addApp
}

TARGET = tst_qudpsocket

symbian: TARGET.CAPABILITY += NetworkServices


CONFIG+=insignificant_test
