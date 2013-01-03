CONFIG += testcase parallel_test

TARGET = tst_qmimedatabase-xml

QT = core testlib concurrent

SOURCES += tst_qmimedatabase-xml.cpp
HEADERS += ../tst_qmimedatabase.h

DEFINES += CORE_SOURCES='"\\"$$QT_SOURCE_TREE/src/corelib\\""'

*-g++*:QMAKE_CXXFLAGS += -W -Wall -Wextra -Wshadow -Wno-long-long -Wnon-virtual-dtor
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
