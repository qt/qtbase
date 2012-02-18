CONFIG += testcase parallel_test

TARGET = tst_qmimedatabase-cache

QT = core testlib concurrent

SOURCES = tst_qmimedatabase-cache.cpp
HEADERS = ../tst_qmimedatabase.h

DEFINES += SRCDIR='"\\"$$PWD/../\\""'

*-g++*:QMAKE_CXXFLAGS += -W -Wall -Wextra -Werror -Wshadow -Wno-long-long -Wnon-virtual-dtor
