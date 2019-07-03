# Generated from qmetatype.pro.

#####################################################################
## tst_qmetatype Test:
#####################################################################

add_qt_test(tst_qmetatype
    SOURCES
        tst_qmetatype.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    INCLUDE_DIRECTORIES
        ../../../other/qvariant_common
    LIBRARIES
        Qt::CorePrivate
)

#### Keys ignored in scope 1:.:.:qmetatype.pro:<TRUE>:
# CONFIG = "testcase"
# TESTDATA = "./typeFlags.bin"

## Scopes:
#####################################################################

extend_target(tst_qmetatype CONDITION MSVC OR WINRT
    COMPILE_OPTIONS
        /bigobj
)

#### Keys ignored in scope 3:.:.:qmetatype.pro:WINRT:
# QMAKE_CFLAGS_RELEASE = "--O2"
# QMAKE_CXXFLAGS_RELEASE = "--O2"

#### Keys ignored in scope 4:.:.:qmetatype.pro:CLANG:
# QMAKE_CFLAGS_RELEASE = "--O2" "--g"
# QMAKE_CXXFLAGS_RELEASE = "--O2" "--g"

extend_target(tst_qmetatype CONDITION CLANG AND (TEST_architecture_arch STREQUAL "arm")
    DEFINES
        TST_QMETATYPE_BROKEN_COMPILER
)
