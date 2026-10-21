# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qlibrary Test:
#####################################################################

# Collect test data
list(APPEND test_data "../library_path/invalid.so")

if(TARGET mylib)
    set(mylib_targets mylib mylib2)
else()
    set(mylib_targets mylib2)
endif()

qt_internal_add_test(tst_qlibrary
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_qlibrary.cpp
    TESTDATA ${test_data}
    LIBRARIES ${mylib_targets} Qt::CorePrivate
)

add_dependencies(tst_qlibrary ${mylib_targets})

if(OHOS)
    set_target_properties(tst_qlibrary PROPERTIES
        BUILD_RPATH_USE_ORIGIN TRUE
    )
endif()

if(ANDROID)
    list(APPEND extra_libs
        "${CMAKE_CURRENT_BINARY_DIR}/../libmylib.so")
    list(APPEND extra_libs
        "${CMAKE_CURRENT_BINARY_DIR}/../libsystem.qt.test.mylib.so")
    set_target_properties(tst_qlibrary PROPERTIES
        QT_ANDROID_EXTRA_LIBS "${extra_libs}")
endif()
