# Generated from qdiriterator.pro.

#####################################################################
## tst_qdiriterator Test:
#####################################################################

# Collect test data
list(APPEND test_data "entrylist")

add_qt_test(tst_qdiriterator
    GUI
    SOURCES
        tst_qdiriterator.cpp
    LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)

# Resources:
add_qt_resource(tst_qdiriterator "qdiriterator"
    PREFIX
        "/testdata/"
    FILES
        "entrylist/directory/dummy"
        "entrylist/file"
)


## Scopes:
#####################################################################

extend_target(tst_qdiriterator CONDITION CONFIG___contains___builtin_testdata
    DEFINES
        BUILTIN_TESTDATA
)
