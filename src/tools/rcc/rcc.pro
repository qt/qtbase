option(host_build)

DEFINES += QT_RCC QT_NO_CAST_FROM_ASCII

include(rcc.pri)
HEADERS += ../../corelib/kernel/qcorecmdlineargs_p.h
SOURCES += main.cpp

load(qt_tool)
