TEMPLATE = lib

TARGET = widgets_snippets

#! [qmake_use]
QT += widgets
#! [qmake_use]

QT += printsupport opengl openglwidgets

SOURCES += customviewstyle.cpp \
           filedialogurls.cpp \
           graphicssceneadditemsnippet.cpp \
           graphicsview.cpp \
           mdiareasnippets.cpp \
           myscrollarea.cpp

load(qt_common)
