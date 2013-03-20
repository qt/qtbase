TARGET = qtforandroid

PLUGIN_TYPE = platforms

# STATICPLUGIN needed because there's a Q_IMPORT_PLUGIN in androidjnimain.cpp
# Yes, the plugin imports itself statically
DEFINES += QT_STATICPLUGIN

load(qt_plugin)

!contains(ANDROID_PLATFORM, android-9) {
    INCLUDEPATH += $$NDK_ROOT/platforms/android-9/arch-$$ANDROID_ARCHITECTURE/usr/include
    LIBS += -L$$NDK_ROOT/platforms/android-9/arch-$$ANDROID_ARCHITECTURE/usr/lib -ljnigraphics -landroid
} else {
    LIBS += -ljnigraphics -landroid
}

include($$PWD/../src/src.pri)
include($$PWD/../src/raster/raster.pri)
