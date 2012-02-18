CONFIG += testcase parallel_test
TARGET = tst_qmimetype
QT = core-private testlib

*-g++*:QMAKE_CXXFLAGS += -W -Wall -Wextra -Werror -Wshadow -Wno-long-long -Wnon-virtual-dtor

SOURCES = tst_qmimetype.cpp
