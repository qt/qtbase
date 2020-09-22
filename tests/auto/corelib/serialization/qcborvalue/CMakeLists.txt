# Generated from qcborvalue.pro.

#####################################################################
## tst_qcborvalue Test:
#####################################################################

qt_internal_add_test(tst_qcborvalue
    SOURCES
        tst_qcborvalue.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
    INCLUDE_DIRECTORIES
        ../../../../../src/3rdparty/tinycbor/src
        ../../../../../src/3rdparty/tinycbor/tests/parser
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)
