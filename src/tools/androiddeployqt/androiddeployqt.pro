option(host_build)
CONFIG += console

SOURCES += \
    main.cpp

# Required for declarations of popen/pclose on Windows
windows: QMAKE_CXXFLAGS += -U__STRICT_ANSI__

DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII
DEFINES += QT_NO_FOREACH

load(qt_app)

