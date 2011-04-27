LIBS += -lwayland-egl -lEGL
INCLUDEPATH += $$PWD
SOURCES += $$PWD/qwaylandeglintegration.cpp \
           $$PWD/qwaylandglcontext.cpp \
           $$PWD/qwaylandeglwindow.cpp \
           $$PWD/../../../eglconvenience/qeglconvenience.cpp

HEADERS += $$PWD/qwaylandeglintegration.h \
           $$PWD/qwaylandglcontext.h \
           $$PWD/qwaylandeglwindow.h \
           $$PWD/../../../eglconvenience/qeglconvenience.h \
    gl_integration/wayland_egl/qwaylandeglinclude.h
