TARGET = QtLinuxAccessibilitySupport
MODULE = linuxaccessibility_support

QT = core-private dbus gui-private accessibility_support-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../../corelib/global/qt_pch.h

include(../../3rdparty/atspi2/atspi2.pri)

HEADERS += \
    application_p.h \
    bridge_p.h \
    cache_p.h  \
    struct_marshallers_p.h \
    constant_mappings_p.h \
    dbusconnection_p.h \
    atspiadaptor_p.h

SOURCES += \
    application.cpp \
    bridge.cpp \
    cache.cpp  \
    struct_marshallers.cpp \
    constant_mappings.cpp \
    dbusconnection.cpp \
    atspiadaptor.cpp

load(qt_module)
