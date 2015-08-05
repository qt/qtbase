TARGET = mirclient
TEMPLATE = lib

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = MirServerIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

QT += core-private gui-private platformsupport-private sensors dbus

CONFIG += qpa/genericunixfontdatabase

DEFINES += MESA_EGL_NO_X11_HEADERS
# CONFIG += c++11 # only enables C++0x
QMAKE_CXXFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden -std=c++11 -Werror -Wall
QMAKE_LFLAGS += -std=c++11 -Wl,-no-undefined

CONFIG += link_pkgconfig
PKGCONFIG += egl mirclient ubuntu-platform-api

SOURCES = \
    backingstore.cpp \
    clipboard.cpp \
    glcontext.cpp \
    input.cpp \
    integration.cpp \
    nativeinterface.cpp \
    platformservices.cpp \
    plugin.cpp \
    screen.cpp \
    theme.cpp \
    window.cpp

HEADERS = \
    backingstore.h \
    clipboard.h \
    glcontext.h \
    input.h \
    integration.h \
    logging.h \
    nativeinterface.h \
    orientationchangeevent_p.h \
    platformservices.h \
    plugin.h \
    screen.h \
    theme.h \
    window.h
