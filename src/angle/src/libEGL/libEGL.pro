TEMPLATE = lib
TARGET = $$qtLibraryTarget(libEGL)

include(../common/common.pri)

# Note: ANGLE is patched to dynamically resolve DwmIsCompositionEnabled DwmSetPresentParameters
# in Surface.cpp, which would otherwise require -ldwmapi, which does not exist on Windows XP
# (QTBUG-27741).

angle_d3d11 {
    LIBS += -ld3d11
} else {
    LIBS += -ld3d9
}
LIBS += -ldxguid -L$$QT_BUILD_TREE/lib -l$$qtLibraryTarget(libGLESv2)

HEADERS += \
    $$ANGLE_DIR/src/libEGL/Config.h \
    $$ANGLE_DIR/src/libEGL/Display.h \
    $$ANGLE_DIR/src/libEGL/main.h \
    $$ANGLE_DIR/src/libEGL/resource.h \
    $$ANGLE_DIR/src/libEGL/ShaderCache.h \
    $$ANGLE_DIR/src/libEGL/Surface.h

SOURCES += \
    $$ANGLE_DIR/src/libEGL/Config.cpp \
    $$ANGLE_DIR/src/libEGL/Display.cpp \
    $$ANGLE_DIR/src/libEGL/libEGL.cpp \
    $$ANGLE_DIR/src/libEGL/main.cpp \
    $$ANGLE_DIR/src/libEGL/Surface.cpp

!static {
    DEF_FILE = $$ANGLE_DIR/src/libEGL/$${TARGET}.def
    win32-g++*:equals(QT_ARCH, i386): DEF_FILE = $$ANGLE_DIR/src/libEGL/$${TARGET}_mingw32.def
}

load(qt_installs)

egl_headers.files = \
    $$ANGLE_DIR/include/EGL/egl.h \
    $$ANGLE_DIR/include/EGL/eglext.h \
    $$ANGLE_DIR/include/EGL/eglplatform.h
egl_headers.path = $$[QT_INSTALL_HEADERS]/QtANGLE/EGL
INSTALLS += egl_headers
