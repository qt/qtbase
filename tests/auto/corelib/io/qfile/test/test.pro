CONFIG += testcase
CONFIG += parallel_test
CONFIG -= app_bundle debug_and_release_target
QT = core-private core testlib
qtHaveModule(network): QT += network
else: DEFINES += QT_NO_NETWORK

TARGET = ../tst_qfile
SOURCES = ../tst_qfile.cpp
wince*: SOURCES += $$QT_SOURCE_TREE/src/corelib/kernel/qfunctions_wince.cpp

RESOURCES += ../qfile.qrc ../rename-fallback.qrc ../copy-fallback.qrc

TESTDATA += ../dosfile.txt ../noendofline.txt ../testfile.txt \
            ../testlog.txt ../two.dots.file ../tst_qfile.cpp \
            ../Makefile ../forCopying.txt ../forRenaming.txt \
            ../resources/file1.ext1

win32:!winrt: LIBS+=-lole32 -luuid
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
