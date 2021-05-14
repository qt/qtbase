# Generated from tst.pro.

#####################################################################
## tst_qlibrary Test:
#####################################################################

# Collect test data
list(APPEND test_data "../library_path/invalid.so")

qt_internal_add_test(tst_qlibrary
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_qlibrary.cpp
    TESTDATA ${test_data}
    LIBRARIES mylib mylib2 # special case
)

## Scopes:
#####################################################################

if(ANDROID)
    # special case begin
    set_source_files_properties(
        ${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so
        PROPERTIES QT_RESOURCE_TARGET_DEPENDENCY mylib
    )
    set_source_files_properties(
        ${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so2
        ${CMAKE_CURRENT_BINARY_DIR}/../system.qt.test.mylib.so
        PROPERTIES QT_RESOURCE_TARGET_DEPENDENCY mylib2
    )
    # special case end
    # Resources:
    set(qmake_libs_resource_files
        # special case begin
        #libmylib.prl
        libmylib.so
        libmylib.so2
        system.qt.test.mylib.so
        # special case end
    )

    list(TRANSFORM qmake_libs_resource_files PREPEND "${CMAKE_CURRENT_BINARY_DIR}/../")

    qt_internal_add_resource(tst_qlibrary "qmake_libs"
        PREFIX
            "android_test_data"
        BASE
            "${CMAKE_CURRENT_BINARY_DIR}/.."
        FILES
            ${qmake_libs_resource_files}
    )
endif()
