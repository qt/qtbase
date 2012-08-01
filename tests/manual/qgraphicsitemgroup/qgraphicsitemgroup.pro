TARGET = qgraphicsitemgroup
QT += widgets
TEMPLATE = app
SOURCES += main.cpp \
    widget.cpp \
    customitem.cpp
HEADERS += widget.h \
    customitem.h
FORMS += widget.ui
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
