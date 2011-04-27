TARGET = qdirectfb
include(../../qpluginbase.pri)
QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms

isEmpty(DIRECTFB_LIBS) {
    DIRECTFB_LIBS = -ldirectfb -lfusion -ldirect -lpthread
}
isEmpty(DIRECTFB_INCLUDEPATH) {
    DIRECTFB_INCLUDEPATH = /usr/include/directfb
}

INCLUDEPATH += $$DIRECTFB_INCLUDEPATH
LIBS += $$DIRECTFB_LIBS

SOURCES = main.cpp \
    qdirectfbintegration.cpp \
    qdirectfbwindowsurface.cpp \
    qdirectfbblitter.cpp \
    qdirectfbconvenience.cpp \
    qdirectfbinput.cpp \
    qdirectfbcursor.cpp \
    qdirectfbwindow.cpp \
    qdirectfbglcontext.cpp
HEADERS = qdirectfbintegration.h \
    qdirectfbwindowsurface.h \
    qdirectfbblitter.h \
    qdirectfbconvenience.h \
    qdirectfbinput.h \
    qdirectfbcursor.h \
    qdirectfbwindow.h \
    qdirectfbglcontext.h

include(../fontdatabases/genericunix/genericunix.pri)
target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target
