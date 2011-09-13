include (../xcomposite_share/xcomposite_share.pri)

LIBS += -lXcomposite -lEGL

SOURCES += \
    $$PWD/qwaylandxcompositeeglcontext.cpp \
    $$PWD/qwaylandxcompositeeglintegration.cpp \
    $$PWD/qwaylandxcompositeeglwindow.cpp

HEADERS += \
    $$PWD/qwaylandxcompositeeglcontext.h \
    $$PWD/qwaylandxcompositeeglintegration.h \
    $$PWD/qwaylandxcompositeeglwindow.h
