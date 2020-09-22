# Generated from qcborstreamreader.pro.

#####################################################################
## tst_qcborstreamreader Test:
#####################################################################

qt_internal_add_test(tst_qcborstreamreader
    SOURCES
        tst_qcborstreamreader.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
    INCLUDE_DIRECTORIES
        ../../../../../src/3rdparty/tinycbor/src
        ../../../../../src/3rdparty/tinycbor/tests/parser
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)
