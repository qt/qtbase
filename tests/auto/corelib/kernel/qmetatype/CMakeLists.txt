# Generated from qmetatype.pro.

#####################################################################
## tst_qmetatype Test:
#####################################################################

# Collect test data
list(APPEND test_data "./typeFlags.bin")

qt_internal_add_test(tst_qmetatype
    SOURCES
        tst_qmetatype.h tst_qmetatype.cpp tst_qmetatype2.cpp
        tst_qmetatype3.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    INCLUDE_DIRECTORIES
        ../../../other/qvariant_common
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
    TESTDATA ${test_data}
)

if(INTEGRITY)
    # Resources:
    set(testdata_resource_files
        "typeFlags.bin"
    )

    qt_internal_add_resource(tst_qmetatype "testdata"
        PREFIX
            "/"
        FILES
            ${testdata_resource_files}
    )
endif()

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmetatype CONDITION MSVC
    COMPILE_OPTIONS
        /bigobj
)

#### Keys ignored in scope 3:.:.:qmetatype.pro:CLANG:
# QMAKE_CFLAGS_RELEASE = "--O2" "--g"
# QMAKE_CXXFLAGS_RELEASE = "--O2" "--g"
