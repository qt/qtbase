TARGET	   = QtSql
QPRO_PWD   = $$PWD
QT         = core
DEFINES += QT_BUILD_SQL_LIB
DEFINES += QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x62000000

unix|win32-g++*:QMAKE_PKGCONFIG_REQUIRES = QtCore

include(../qbase.pri)

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../corelib/global/qt_pch.h
SQL_P = sql

include(kernel/kernel.pri)
include(drivers/drivers.pri)
include(models/models.pri)

symbian: {
    TARGET.UID3=0x2001E61D

    # Problems using data exports from this DLL mean that we can't page it on releases that don't support
    # data exports (currently that's any release before Symbian^3)
    pagingBlock = "$${LITERAL_HASH}ifndef SYMBIAN_DLL_DATA_EXPORTS_SUPPORTED" \
                  "UNPAGED" \
                  "$${LITERAL_HASH}endif"
    MMP_RULES += pagingBlock
}
