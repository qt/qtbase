CONFIG += testcase parallel_test

TARGET = tst_qmimedatabase-xml

QT = core testlib concurrent

SOURCES += tst_qmimedatabase-xml.cpp
HEADERS += ../tst_qmimedatabase.h

RESOURCES += $$QT_SOURCE_TREE/src/corelib/mimetypes/mimetypes.qrc
RESOURCES += ../testdata.qrc

*-g++*:QMAKE_CXXFLAGS += -W -Wall -Wextra -Wshadow -Wno-long-long -Wnon-virtual-dtor
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

unix:!mac:!qnx: DEFINES += USE_XDG_DATA_DIRS
