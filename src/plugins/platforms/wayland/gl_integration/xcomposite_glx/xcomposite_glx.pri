include (../xcomposite_share/xcomposite_share.pri)
include (../../../glxconvenience/glxconvenience.pri)

LIBS += -lXcomposite
SOURCES += \
    $$PWD/qwaylandxcompositeglxcontext.cpp \
    $$PWD/qwaylandxcompositeglxintegration.cpp \
    $$PWD/qwaylandxcompositeglxwindow.cpp

HEADERS += \
    $$PWD/qwaylandxcompositeglxcontext.h \
    $$PWD/qwaylandxcompositeglxintegration.h \
    $$PWD/qwaylandxcompositeglxwindow.h
