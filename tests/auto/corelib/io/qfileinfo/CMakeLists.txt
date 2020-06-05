# Generated from qfileinfo.pro.

#####################################################################
## tst_qfileinfo Test:
#####################################################################

add_qt_test(tst_qfileinfo
    SOURCES
        tst_qfileinfo.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

# Resources:
set(qfileinfo_resource_files
    "resources/"
)

add_qt_resource(tst_qfileinfo "qfileinfo"
    PREFIX
        "/tst_qfileinfo/"
    FILES
        ${qfileinfo_resource_files}
)
set(testdata_resource_files
    "resources/file1"
    "resources/file1.ext1"
    "resources/file1.ext1.ext2"
    "tst_qfileinfo.cpp"
)

add_qt_resource(tst_qfileinfo "testdata"
    PREFIX
        "/testdata"
    FILES
        ${testdata_resource_files}
)


## Scopes:
#####################################################################

extend_target(tst_qfileinfo CONDITION WIN32
    PUBLIC_LIBRARIES
        advapi32
        netapi32
)
