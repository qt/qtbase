requires(qtHaveModule(widgets))
requires(qtHaveModule(printsupport))

TEMPLATE = app

TARGET = widgets_snippets

QT += widgets printsupport

SOURCES += customviewstyle.cpp \
           filedialogurls.cpp \
           graphicssceneadditemsnippet.cpp \
           graphicsview.cpp \
           mdiareasnippets.cpp \
           myscrollarea.cpp

