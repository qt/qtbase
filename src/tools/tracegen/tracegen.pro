option(host_build)
CONFIG += force_bootstrap

SOURCES += \
    etw.cpp \
    helpers.cpp \
    lttng.cpp \
    panic.cpp \
    provider.cpp \
    qtheaders.cpp \
    tracegen.cpp

HEADERS += \
    etw.h \
    helpers.h \
    lttng.h \
    panic.h \
    provider.h \
    qtheaders.h

load(qt_tool)
