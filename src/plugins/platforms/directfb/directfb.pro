TARGET = qdirectfb
load(qt_plugin)
DESTDIR = $$QT.gui.plugins/platforms

QT += core-private gui-private platformsupport-private

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
    qdirectfbbackingstore.cpp \
    qdirectfbblitter.cpp \
    qdirectfbconvenience.cpp \
    qdirectfbinput.cpp \
    qdirectfbcursor.cpp \
    qdirectfbwindow.cpp \
    qdirectfbscreen.cpp
HEADERS = qdirectfbintegration.h \
    qdirectfbbackingstore.h \
    qdirectfbblitter.h \
    qdirectfbconvenience.h \
    qdirectfbinput.h \
    qdirectfbcursor.h \
    qdirectfbwindow.h \
    qdirectfbscreen.h \
    qdirectfbeglhooks.h

# ### port the GL context
directfb_egl {
    HEADERS += qdirectfb_egl.h
    SOURCES += qdirectfb_egl.cpp
    DEFINES += DIRECTFB_GL_EGL
}

!isEmpty(DIRECTFB_PLATFORM_HOOKS_SOURCES) {
    HEADERS += $$DIRECTFB_PLATFORM_HOOKS_HEADERS
    SOURCES += $$DIRECTFB_PLATFORM_HOOKS_SOURCES
    DEFINES += DIRECTFB_PLATFORM_HOOKS
} else {
    SOURCES += qdirectfbeglhooks_stub.cpp
}


CONFIG += qpa/genericunixfontdatabase
target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target

OTHER_FILES += directfb.json
