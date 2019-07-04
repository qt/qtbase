# Generated from qdiriterator.pro.

#####################################################################
## tst_qdiriterator Test:
#####################################################################

add_qt_test(tst_qdiriterator
    SOURCES
        tst_qdiriterator.cpp
    LIBRARIES
        Qt::CorePrivate
)

# Resources:
add_qt_resource(tst_qdiriterator "qdiriterator" PREFIX "/testdata/" FILES
    entrylist/directory/dummy
    entrylist/file)


#### Keys ignored in scope 1:.:.:qdiriterator.pro:<TRUE>:
# CONFIG = "testcase"
# TESTDATA = "entrylist"

## Scopes:
#####################################################################

extend_target(tst_qdiriterator CONDITION CONFIG___contains___builtin_testdata
    DEFINES
        BUILTIN_TESTDATA
)
