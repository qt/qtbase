QT += testlib
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
SOURCES = main.cpp \
    touchwidget.cpp
FORMS += form.ui
HEADERS += touchwidget.h
