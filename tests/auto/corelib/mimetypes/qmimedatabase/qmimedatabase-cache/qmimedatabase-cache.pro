CONFIG += testcase

TARGET = tst_qmimedatabase-cache

QT = core testlib concurrent

SOURCES = tst_qmimedatabase-cache.cpp
HEADERS = ../tst_qmimedatabase.h
RESOURCES += $$QT_SOURCE_TREE/src/corelib/mimetypes/mimetypes.qrc
RESOURCES += ../testdata.qrc

*-g++*:QMAKE_CXXFLAGS += -W -Wall -Wextra -Wshadow -Wno-long-long -Wnon-virtual-dtor

unix:!mac:!qnx: DEFINES += USE_XDG_DATA_DIRS
