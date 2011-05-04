include (../../../eglconvenience/eglconvenience.pri)

LIBS += -lwayland-egl -lEGL
INCLUDEPATH += $$PWD
SOURCES += $$PWD/qwaylandeglintegration.cpp \
           $$PWD/qwaylandglcontext.cpp \
           $$PWD/qwaylandeglwindow.cpp

HEADERS += $$PWD/qwaylandeglintegration.h \
           $$PWD/qwaylandglcontext.h \
           $$PWD/qwaylandeglwindow.h
