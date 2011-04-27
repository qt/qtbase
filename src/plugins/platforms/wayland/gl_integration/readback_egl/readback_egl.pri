
LIBS += -lX11 -lXext -lEGL

HEADERS += \
    $$PWD/qwaylandreadbackeglintegration.h \
    $$PWD/qwaylandreadbackeglcontext.h \
    $$PWD/qwaylandreadbackeglwindow.h \
    $$PWD/../../../eglconvenience/qeglconvenience.h

SOURCES += \
    $$PWD/qwaylandreadbackeglintegration.cpp \
    $$PWD/qwaylandreadbackeglwindow.cpp \
    $$PWD/qwaylandreadbackeglcontext.cpp \
    $$PWD/../../../eglconvenience/qeglconvenience.cpp
