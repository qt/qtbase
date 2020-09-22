# Generated from qdiriterator.pro.

#####################################################################
## tst_qdiriterator Test:
#####################################################################

# Collect test data
list(APPEND test_data "entrylist")

qt_internal_add_test(tst_qdiriterator
    SOURCES
        tst_qdiriterator.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)

# Resources:
set(qdiriterator_resource_files
    "entrylist/directory/dummy"
    "entrylist/file"
)

qt_internal_add_resource(tst_qdiriterator "qdiriterator"
    PREFIX
        "/testdata/"
    FILES
        ${qdiriterator_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qdiriterator CONDITION CONFIG___contains___builtin_testdata
    DEFINES
        BUILTIN_TESTDATA
)
