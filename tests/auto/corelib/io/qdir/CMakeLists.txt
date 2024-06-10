# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdir Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qdir LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
list(APPEND test_data_dirs "testdir")
list(APPEND test_data_dirs "testData")
list(APPEND test_data_dirs "searchdir")
list(APPEND test_data_dirs "resources")
list(APPEND test_data_dirs "entrylist")
list(APPEND test_data_dirs "types")
list(APPEND test_data_dirs "tst_qdir.cpp")

set(test_data_files)

foreach(dir  ${test_data_dirs})
    set(out)
    file(GLOB_RECURSE out true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${dir}/*)
    if (NOT out)
        set(out ${dir})
    endif()
    set(test_data_files ${test_data_files} ${out})
endforeach()

set(android_testdata_resource_files
"entrylist/directory/dummy"
"entrylist/file"
"resources/entryList/file1.data"
"resources/entryList/file2.data"
"resources/entryList/file3.data"
"resources/entryList/file4.nothing"
"searchdir/subdir1/picker.png"
"searchdir/subdir2/picker.png"
"testData/empty"
"testdir/dir/qdir.pro"
"testdir/dir/qrc_qdir.cpp"
"testdir/dir/tmp/empty"
"testdir/dir/tst_qdir.cpp"
"testdir/spaces/foo. bar"
"testdir/spaces/foo.bar"
"tst_qdir.cpp"
"types/a"
"types/a.a"
"types/a.b"
"types/a.c"
"types/b"
"types/b.a"
"types/b.b"
"types/b.c"
"types/c"
"types/c.a"
"types/c.b"
"types/c.c"
"types/d.a/dummy"
"types/d.b/dummy"
"types/d.c/dummy"
"types/d/dummy"
"types/e.a/dummy"
"types/e.b/dummy"
"types/e.c/dummy"
"types/e/dummy"
"types/f.a/dummy"
"types/f.b/dummy"
"types/f.c/dummy"
"types/f/dummy"
)


if (WASM)
    qt_internal_add_test(tst_qdir
        SOURCES
            tst_qdir.cpp
        LIBRARIES
            Qt::CorePrivate
            Qt::TestPrivate
        TESTDATA
    )
    qt_internal_add_resource(tst_qdir "tst_qdir"
        PREFIX
            "/tst_qdir"
        FILES
            ${test_data_files}
    )
elseif (ANDROID)
    qt_internal_add_test(tst_qdir
        SOURCES
            tst_qdir.cpp
        LIBRARIES
            Qt::CorePrivate
            Qt::TestPrivate
        TESTDATA ${test_data_dirs}
    )

    # Resources:
    set(qdir_resource_files
        "resources/entryList/"
    )

    qt_internal_add_resource(tst_qdir "qdir"
        PREFIX
            "/tst_qdir/"
        FILES
            ${qdir_resource_files}
    )

    qt_internal_add_resource(tst_qdir "android_testdata"
        PREFIX
            "/android_testdata"
        FILES
            ${android_testdata_resource_files}
    )

else()

    qt_internal_add_test(tst_qdir
        SOURCES
            tst_qdir.cpp
        LIBRARIES
            Qt::CorePrivate
            Qt::TestPrivate
        TESTDATA ${test_data_dirs}
    )

    # Resources:
    set(qdir_resource_files
        "resources/entryList/"
    )

    qt_internal_add_resource(tst_qdir "qdir"
        PREFIX
            "/tst_qdir/"
        FILES
            ${qdir_resource_files}
    )
endif()

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qdir CONDITION CONFIG___contains___builtin_testdata
    DEFINES
        BUILTIN_TESTDATA
)
