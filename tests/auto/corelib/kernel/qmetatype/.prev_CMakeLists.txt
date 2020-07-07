# Generated from qmetatype.pro.

#####################################################################
## tst_qmetatype Test:
#####################################################################

# Collect test data
list(APPEND test_data "./typeFlags.bin")

qt_add_test(tst_qmetatype
    SOURCES
        tst_qmetatype.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    INCLUDE_DIRECTORIES
        ../../../other/qvariant_common
    PUBLIC_LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_extend_target(tst_qmetatype CONDITION MSVC
    COMPILE_OPTIONS
        /bigobj
)

#### Keys ignored in scope 3:.:.:qmetatype.pro:CLANG:
# QMAKE_CFLAGS_RELEASE = "--O2" "--g"
# QMAKE_CXXFLAGS_RELEASE = "--O2" "--g"

qt_extend_target(tst_qmetatype CONDITION CLANG AND (TEST_architecture_arch STREQUAL "arm")
    DEFINES
        TST_QMETATYPE_BROKEN_COMPILER
)
