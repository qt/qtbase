TARGET = QtXkbCommonSupport
MODULE = xkbcommon_support

QT = core-private gui-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../../../corelib/global/qt_pch.h

QMAKE_USE += xkbcommon

HEADERS += \
    qxkbcommon_p.h

SOURCES += \
    qxkbcommon.cpp \
    qxkbcommon_3rdparty.cpp

# qxkbcommon.cpp::KeyTbl has more than 256 levels of expansion and older
# Clang uses that as a limit (it's 1024 in current versions).
clang:!intel_icc: QMAKE_CXXFLAGS += -ftemplate-depth=1024

load(qt_module)
