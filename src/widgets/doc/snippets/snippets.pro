TEMPLATE = lib

TARGET = widgets_snippets

QT += widgets printsupport

SOURCES += customviewstyle/customviewstyle.cpp \
           filedialogurls/filedialogurls.cpp \
           graphicssceneadditem/graphicssceneadditemsnippet.cpp \
           graphicsview/graphicsview.cpp \
           mdiarea/mdiareasnippets.cpp \
           myscrollarea/myscrollarea.cpp

load(qt_common)
