TARGET = bearermonitor
QT = core gui network widgets
requires(qtConfig(treeview))

HEADERS = sessionwidget.h \
          bearermonitor.h

SOURCES = main.cpp \
          bearermonitor.cpp \
          sessionwidget.cpp

FORMS = bearermonitor_240_320.ui \
        bearermonitor_640_480.ui \
        sessionwidget.ui

win32:LIBS += -lws2_32

CONFIG += console

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/bearermonitor
INSTALLS += target
