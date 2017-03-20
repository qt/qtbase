# The device integration plugin base class has to live in a shared library,
# placing it into a static lib like platformsupport is not sufficient since we
# have to keep the QObject magic like qobject_cast working.
# Hence this private-only module.
# By having _p headers, it also enables developing out-of-tree integration plugins.

TARGET = QtEglFSDeviceIntegration
CONFIG += internal_module
MODULE = eglfsdeviceintegration

QT += \
    core-private gui-private \
    devicediscovery_support-private eventdispatcher_support-private \
    service_support-private theme_support-private fontdatabase_support-private \
    fb_support-private egl_support-private

qtHaveModule(input_support-private): \
    QT += input_support-private

qtHaveModule(platformcompositor_support-private): \
    QT += platformcompositor_support-private

# Avoid X11 header collision, use generic EGL native types
DEFINES += QT_EGL_NO_X11

DEFINES += QT_BUILD_EGL_DEVICE_LIB

include($$PWD/api/api.pri)

QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

!isEmpty(EGLFS_PLATFORM_HOOKS_SOURCES) {
    HEADERS += $$EGLFS_PLATFORM_HOOKS_HEADERS
    SOURCES += $$EGLFS_PLATFORM_HOOKS_SOURCES
    LIBS    += $$EGLFS_PLATFORM_HOOKS_LIBS
    DEFINES += EGLFS_PLATFORM_HOOKS
}

!isEmpty(EGLFS_DEVICE_INTEGRATION) {
    DEFINES += EGLFS_PREFERRED_PLUGIN=$$EGLFS_DEVICE_INTEGRATION
}

CONFIG += egl

# Prevent gold linker from crashing.
# This started happening when QtPlatformSupport was modularized.
use_gold_linker: CONFIG += no_linker_version_script

!contains(DEFINES, QT_NO_CURSOR): RESOURCES += $$PWD/cursor.qrc

load(qt_module)
