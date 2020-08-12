option(host_build)
CONFIG += force_bootstrap

DEFINES += \
    QT_MOC \
    QT_NO_CAST_FROM_ASCII \
    QT_NO_CAST_FROM_BYTEARRAY \
    QT_NO_COMPRESS \
    QT_NO_FOREACH

# strerror() is safe to use because moc is single-threaded
msvc: DEFINES += _CRT_SECURE_NO_WARNINGS

include(moc.pri)
HEADERS += qdatetime_p.h
SOURCES += main.cpp

QMAKE_TARGET_DESCRIPTION = "Qt Meta Object Compiler"
load(qt_tool)
