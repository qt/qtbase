requires(qtHaveModule(widgets))
requires(qtHaveModule(printsupport))

TEMPLATE = lib

TARGET = widgets_snippets

QT += widgets printsupport opengl openglwidgets

SOURCES += customviewstyle.cpp \
           filedialogurls.cpp \
           graphicssceneadditemsnippet.cpp \
           graphicsview.cpp \
           mdiareasnippets.cpp \
           myscrollarea.cpp

