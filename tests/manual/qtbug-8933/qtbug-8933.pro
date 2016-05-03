TARGET = qtbug-8933
TEMPLATE = app
QT += widgets

SOURCES += main.cpp\
        widget.cpp

HEADERS  += widget.h

FORMS    += widget.ui
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
