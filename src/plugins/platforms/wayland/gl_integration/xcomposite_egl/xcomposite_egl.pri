include (../xcomposite_share/xcomposite_share.pri)
load(qpa/egl/convenience)
load(qpa/egl/context)
load(qpa/egl/xlibintegration)

LIBS += -lXcomposite -lEGL

SOURCES += \
    $$PWD/qwaylandxcompositeeglcontext.cpp \
    $$PWD/qwaylandxcompositeeglintegration.cpp \
    $$PWD/qwaylandxcompositeeglwindow.cpp

HEADERS += \
    $$PWD/qwaylandxcompositeeglcontext.h \
    $$PWD/qwaylandxcompositeeglintegration.h \
    $$PWD/qwaylandxcompositeeglwindow.h
