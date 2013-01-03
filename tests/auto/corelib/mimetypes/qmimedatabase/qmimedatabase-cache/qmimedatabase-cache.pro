CONFIG += testcase

TARGET = tst_qmimedatabase-cache

QT = core testlib concurrent

SOURCES = tst_qmimedatabase-cache.cpp
HEADERS = ../tst_qmimedatabase.h

DEFINES += CORE_SOURCES='"\\"$$QT_SOURCE_TREE/src/corelib\\""'

*-g++*:QMAKE_CXXFLAGS += -W -Wall -Wextra -Wshadow -Wno-long-long -Wnon-virtual-dtor
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
