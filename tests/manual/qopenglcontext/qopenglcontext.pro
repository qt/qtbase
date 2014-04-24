TEMPLATE = app
TARGET = qopenglcontext

QT += gui-private platformsupport-private

HEADERS += $$PWD/qopenglcontextwindow.h

SOURCES += $$PWD/main.cpp \
           $$PWD/qopenglcontextwindow.cpp
