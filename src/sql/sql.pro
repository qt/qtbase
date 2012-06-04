load(qt_module)

TARGET	   = QtSql
QPRO_PWD   = $$PWD
QT         = core-private

DEFINES += QT_BUILD_SQL_LIB
DEFINES += QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x62000000

unix|win32-g++*:QMAKE_PKGCONFIG_REQUIRES = QtCore

load(qt_module_config)

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../corelib/global/qt_pch.h
SQL_P = sql

QMAKE_DOCS = $$PWD/doc/qtsql.qdocconf
QMAKE_DOCS_INDEX = ../../doc

include(kernel/kernel.pri)
include(drivers/drivers.pri)
include(models/models.pri)
