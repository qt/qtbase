TARGET = bearermonitor
QT = core gui network

HEADERS = sessionwidget.h \
          bearermonitor.h

SOURCES = main.cpp \
          bearermonitor.cpp \
          sessionwidget.cpp

maemo5|maemo6|linux-g++-maemo {
  DEFINES += MAEMO_UI
  FORMS = bearermonitor_maemo.ui \
          sessionwidget_maemo.ui
} else {
  FORMS = bearermonitor_240_320.ui \
          bearermonitor_640_480.ui \
          sessionwidget.ui
}

win32:!wince*:LIBS += -lws2_32
wince*:LIBS += -lws2

CONFIG += console

symbian: {
    TARGET.CAPABILITY = NetworkServices ReadUserData
    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
}
maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
