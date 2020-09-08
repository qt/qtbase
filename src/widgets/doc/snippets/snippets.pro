TEMPLATE = lib

TARGET = widgets_snippets

#! [qmake_use]
QT += widgets
#! [qmake_use]

QT += printsupport opengl openglwidgets

SOURCES += customviewstyle/customviewstyle.cpp \
           filedialogurls/filedialogurls.cpp \
           graphicssceneadditem/graphicssceneadditemsnippet.cpp \
           graphicsview/graphicsview.cpp \
           mdiarea/mdiareasnippets.cpp \
           myscrollarea/myscrollarea.cpp

load(qt_common)
