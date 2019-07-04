# Generated from qfileinfo.pro.

#####################################################################
## tst_qfileinfo Test:
#####################################################################

add_qt_test(tst_qfileinfo
    SOURCES
        tst_qfileinfo.cpp
    LIBRARIES
        Qt::CorePrivate
)

# Resources:
add_qt_resource(tst_qfileinfo "qfileinfo" PREFIX "/tst_qfileinfo/" FILES
    resources/)
add_qt_resource(tst_qfileinfo "testdata" PREFIX "/testdata" FILES
    resources/file1
    resources/file1.ext1
    resources/file1.ext1.ext2
    tst_qfileinfo.cpp)


#### Keys ignored in scope 1:.:.:qfileinfo.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

extend_target(tst_qfileinfo CONDITION WIN32 AND NOT WINRT
    PUBLIC_LIBRARIES
        advapi32
        netapi32
)
