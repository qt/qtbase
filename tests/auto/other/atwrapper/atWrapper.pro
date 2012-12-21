ARTHUR=$$QT_SOURCE_TREE/tests/arthur
COMMON_FOLDER = $$ARTHUR/common
include($$ARTHUR/arthurtester.pri)
INCLUDEPATH += $$ARTHUR
DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += xml svg network testlib

qtHaveModule(opengl): QT += opengl

include($$ARTHUR/datagenerator/datagenerator.pri)

CONFIG += testcase

HEADERS += atWrapper.h
SOURCES += atWrapperAutotest.cpp atWrapper.cpp

TARGET = tst_atwrapper

#include($$COMMON_FOLDER/common.pri)
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
