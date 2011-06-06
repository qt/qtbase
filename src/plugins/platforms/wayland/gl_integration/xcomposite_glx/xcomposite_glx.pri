include (../xcomposite_share/xcomposite_share.pri)
load(qpa/glx/convenience)

LIBS += -lXcomposite
SOURCES += \
    $$PWD/qwaylandxcompositeglxcontext.cpp \
    $$PWD/qwaylandxcompositeglxintegration.cpp \
    $$PWD/qwaylandxcompositeglxwindow.cpp

HEADERS += \
    $$PWD/qwaylandxcompositeglxcontext.h \
    $$PWD/qwaylandxcompositeglxintegration.h \
    $$PWD/qwaylandxcompositeglxwindow.h
