SOURCES = objcopy.cpp
CONFIG -= qt
TARGET = objcopytest

load(resolve_target)

QMAKE_POST_LINK += $$QMAKE_OBJCOPY --only-keep-debug $$QMAKE_RESOLVED_TARGET objcopytest.debug && $$QMAKE_OBJCOPY --strip-debug $$QMAKE_RESOLVED_TARGET && $$QMAKE_OBJCOPY --add-gnu-debuglink=objcopytest.debug $$QMAKE_RESOLVED_TARGET
