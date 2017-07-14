option(host_build)
CONFIG += force_bootstrap

DEFINES += QT_RCC QT_NO_CAST_FROM_ASCII QT_NO_FOREACH

include(rcc.pri)
SOURCES += main.cpp

QMAKE_TARGET_DESCRIPTION = "Qt Resource Compiler"
load(qt_tool)
