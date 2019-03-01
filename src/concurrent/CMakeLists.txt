# Generated from concurrent.pro.

#####################################################################
## Concurrent Module:
#####################################################################

add_qt_module(Concurrent
    SOURCES
        qtconcurrent_global.h
        qtconcurrentcompilertest.h
        qtconcurrentexception.h
        qtconcurrentfilter.cpp qtconcurrentfilter.h
        qtconcurrentfilterkernel.h
        qtconcurrentfunctionwrappers.h
        qtconcurrentiteratekernel.cpp qtconcurrentiteratekernel.h
        qtconcurrentmap.cpp qtconcurrentmap.h
        qtconcurrentmapkernel.h
        qtconcurrentmedian.h
        qtconcurrentreducekernel.h
        qtconcurrentrun.cpp qtconcurrentrun.h
        qtconcurrentrunbase.h
        qtconcurrentstoredfunctioncall.h
        qtconcurrentthreadengine.cpp qtconcurrentthreadengine.h
    DEFINES
        QT_NO_USING_NAMESPACE
        QT_NO_FOREACH
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES  # special case
        Qt::Core
    # CONFIG = "exceptions"
    # PRECOMPILED_HEADER = "../corelib/global/qt_pch.h"
    # _LOADED = "qt_module"
)

## Scopes:
#####################################################################

extend_target(Concurrent CONDITION MSVC AND (TEST_architecture STREQUAL "i386")
    LINK_OPTIONS
        "/BASE:0x66000000"
)

add_qt_docs(./doc/qtconcurrent.qdocconf)
