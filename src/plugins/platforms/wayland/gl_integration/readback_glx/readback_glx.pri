include (../../../glxconvenience/glxconvenience.pri)
HEADERS += \
    $$PWD/qwaylandreadbackglxintegration.h \
    $$PWD/qwaylandreadbackglxwindow.h \
    $$PWD/qwaylandreadbackglxcontext.h

SOURCES += \
    $$PWD/qwaylandreadbackglxintegration.cpp \
    $$PWD/qwaylandreadbackglxwindow.cpp \
    $$PWD/qwaylandreadbackglxcontext.cpp

LIBS += -lX11
