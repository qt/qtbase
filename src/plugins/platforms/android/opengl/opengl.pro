TARGET = qtforandroidGL

PLUGIN_TYPE = platforms
load(qt_plugin)

# STATICPLUGIN needed because there's a Q_IMPORT_PLUGIN in androidjnimain.cpp
# Yes, the plugin imports itself statically
DEFINES += QT_STATICPLUGIN ANDROID_PLUGIN_OPENGL

!equals(ANDROID_PLATFORM, android-9) {
    INCLUDEPATH += $$NDK_ROOT/platforms/android-9/arch-$$ANDROID_ARCHITECTURE/usr/include
    LIBS += -L$$NDK_ROOT/platforms/android-9/arch-$$ANDROID_ARCHITECTURE/usr/lib -ljnigraphics -landroid
} else {
    LIBS += -ljnigraphics -landroid
}

EGLFS_PLATFORM_HOOKS_SOURCES = $$PWD/../src/opengl/qeglfshooks_android.cpp

INCLUDEPATH += $$PWD/../src/opengl/

HEADERS += \
    $$PWD/../src/opengl/qandroidopenglcontext.h \
    $$PWD/../src/opengl/qandroidopenglplatformwindow.h

SOURCES += \
    $$PWD/../src/opengl/qandroidopenglcontext.cpp \
    $$PWD/../src/opengl/qandroidopenglplatformwindow.cpp

include($$PWD/../../eglfs/eglfs.pri)
include($$PWD/../src/src.pri)
