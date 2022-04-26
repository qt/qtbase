# Generated from qdebug.pro.

#####################################################################
## tst_qdebug Test:
#####################################################################

qt_internal_add_test(tst_qdebug
    SOURCES
        tst_qdebug.cpp
    PUBLIC_LIBRARIES
        Qt::Concurrent
)

if (APPLE)
    target_compile_options(tst_qdebug PRIVATE -x objective-c++)
endif()
