TARGET = QtFbSupport
MODULE = fb_support

QT = core-private gui-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../../corelib/global/qt_pch.h

SOURCES += \
    qfbscreen.cpp \
    qfbbackingstore.cpp \
    qfbwindow.cpp \
    qfbcursor.cpp \
    qfbvthandler.cpp

HEADERS += \
    qfbscreen_p.h \
    qfbbackingstore_p.h \
    qfbwindow_p.h \
    qfbcursor_p.h \
    qfbvthandler_p.h

load(qt_module)
