option(host_build)
!force_bootstrap:if(!qtConfig(commandlineparser)|!qtConfig(textcodec)): \
    CONFIG += force_bootstrap

DEFINES += QT_UIC QT_NO_CAST_FROM_ASCII QT_NO_FOREACH

include(uic.pri)
include(cpp/cpp.pri)

HEADERS += uic.h

SOURCES += main.cpp \
           uic.cpp

load(qt_tool)
