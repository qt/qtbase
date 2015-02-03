option(host_build)

DEFINES += QT_UIC QT_NO_CAST_FROM_ASCII

include(uic.pri)
include(cpp/cpp.pri)

HEADERS += uic.h

SOURCES += main.cpp \
           uic.cpp

load(qt_tool)
