QT += widgets widgets-private gui-private core-private

TARGET = stereographicsview
TEMPLATE = app

SOURCES += main.cpp \
        mainwindow.cpp \
        mainwindow.h \
        mainwindow.ui \
        mygraphicsview.h \
        mygraphicsview.cpp \

HEADERS  += openglwidget.h
