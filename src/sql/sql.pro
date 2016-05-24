TARGET	   = QtSql
QT         = core-private

DEFINES += QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x62000000

QMAKE_DOCS = $$PWD/doc/qtsql.qdocconf

DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_FOREACH
PRECOMPILED_HEADER = ../corelib/global/qt_pch.h
SQL_P = sql

include(kernel/kernel.pri)
include(models/models.pri)

MODULE_PLUGIN_TYPES = \
    sqldrivers
load(qt_module)
