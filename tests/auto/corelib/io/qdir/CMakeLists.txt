# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdir Test:
#####################################################################

# Collect test data
list(APPEND test_data "testdir")
list(APPEND test_data "testData")
list(APPEND test_data "searchdir")
list(APPEND test_data "resources")
list(APPEND test_data "entrylist")
list(APPEND test_data "types")
list(APPEND test_data "tst_qdir.cpp")

qt_internal_add_test(tst_qdir
    SOURCES
        tst_qdir.cpp
    LIBRARIES
        Qt::CorePrivate
    TESTDATA ${test_data}
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

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qdir CONDITION CONFIG___contains___builtin_testdata
    DEFINES
        BUILTIN_TESTDATA
)

if(ANDROID)
    # Resources:
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

    qt_internal_add_resource(tst_qdir "android_testdata"
        PREFIX
            "/android_testdata"
        FILES
            ${android_testdata_resource_files}
    )
endif()
