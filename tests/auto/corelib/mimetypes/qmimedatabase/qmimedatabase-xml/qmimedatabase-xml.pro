CONFIG += testcase parallel_test

TARGET = tst_qmimedatabase-xml

QT = core testlib concurrent

CONFIG += depend_includepath

SOURCES += tst_qmimedatabase-xml.cpp
HEADERS += ../tst_qmimedatabase.h

DEFINES += SRCDIR='"\\"$$PWD/../\\""'

*-g++*:QMAKE_CXXFLAGS += -W -Wall -Wextra -Werror -Wshadow -Wno-long-long -Wnon-virtual-dtor
