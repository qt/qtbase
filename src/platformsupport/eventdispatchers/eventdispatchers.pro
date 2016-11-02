TARGET = QtEventDispatcherSupport
MODULE = eventdispatcher_support

QT = core-private gui-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../../corelib/global/qt_pch.h

unix {
    SOURCES += \
        qunixeventdispatcher.cpp \
        qgenericunixeventdispatcher.cpp

    HEADERS += \
        qunixeventdispatcher_qpa_p.h \
        qgenericunixeventdispatcher_p.h
} else {
    SOURCES += \
        qwindowsguieventdispatcher.cpp

    HEADERS += \
        qwindowsguieventdispatcher_p.h
}

qtConfig(glib) {
    SOURCES += qeventdispatcher_glib.cpp
    HEADERS += qeventdispatcher_glib_p.h
    QMAKE_USE_PRIVATE += glib
}

load(qt_module)
