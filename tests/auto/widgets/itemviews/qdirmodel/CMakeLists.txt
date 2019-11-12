# Generated from qdirmodel.pro.

#####################################################################
## tst_qdirmodel Test:
#####################################################################

add_qt_test(tst_qdirmodel
    SOURCES
        ../../../../shared/emulationdetector.h
        tst_qdirmodel.cpp
    INCLUDE_DIRECTORIES
        ../../../../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

## Scopes:
#####################################################################

extend_target(tst_qdirmodel CONDITION ANDROID
    DEFINES
        SRCDIR=\\\"./\\\"
)

extend_target(tst_qdirmodel CONDITION NOT ANDROID
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}/\\\"
)

if(ANDROID AND NOT ANDROID_EMBEDDED)
    # Resources:
    set(testdata_resource_files
        "dirtest/test1/dummy"
        "dirtest/test1/test"
        "test/file01.tst"
        "test/file02.tst"
        "test/file03.tst"
        "test/file04.tst"
        "tst_qdirmodel.cpp"
    )

    add_qt_resource(tst_qdirmodel "testdata"
        PREFIX
            "/android_testdata"
        FILES
            ${testdata_resource_files}
    )
endif()
