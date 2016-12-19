TARGET = qdirectfb

QT += \
    core-private gui-private \
    eventdispatcher_support-private service_support-private \
    fontdatabase_support-private

QMAKE_USE += directfb

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
contains(QT_CONFIG, directfb_egl) {
    QT += egl_support-private
    HEADERS += qdirectfb_egl.h
    SOURCES += qdirectfb_egl.cpp
    DEFINES += DIRECTFB_GL_EGL
}

!isEmpty(DIRECTFB_PLATFORM_HOOKS_SOURCES) {
    HEADERS += $$DIRECTFB_PLATFORM_HOOKS_HEADERS
    SOURCES += $$DIRECTFB_PLATFORM_HOOKS_SOURCES
    DEFINES += DIRECTFB_PLATFORM_HOOKS
    LIBS += $$DIRECTFB_PLATFORM_HOOKS_LIBS
    QMAKE_LIBDIR += $$DIRECTFB_PLATFORM_HOOKS_LIBDIR
    INCLUDEPATH += $$DIRECTFB_PLATFORM_HOOKS_INCLUDEPATH
} else {
    SOURCES += qdirectfbeglhooks_stub.cpp
}

OTHER_FILES += directfb.json

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QDirectFbIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
