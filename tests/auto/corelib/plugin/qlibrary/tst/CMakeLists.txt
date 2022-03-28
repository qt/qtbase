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

add_dependencies(tst_qlibrary mylib mylib2)

if(ANDROID)
    list(APPEND extra_libs
        "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so")
    list(APPEND extra_libs
        "${CMAKE_CURRENT_BINARY_DIR}/../libsystem.qt.test.mylib.so")
    set_target_properties(tst_qlibrary PROPERTIES
        QT_ANDROID_EXTRA_LIBS "${extra_libs}")
    qt6_android_generate_deployment_settings(tst_qlibrary)
endif()
