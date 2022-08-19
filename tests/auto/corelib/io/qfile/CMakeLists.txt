# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfile Test:
#####################################################################

# Collect test data
list(APPEND test_data "dosfile.txt")
list(APPEND test_data "noendofline.txt")
list(APPEND test_data "testfile.txt")
list(APPEND test_data "testlog.txt")
list(APPEND test_data "two.dots.file")
list(APPEND test_data "tst_qfile.cpp")
list(APPEND test_data "forCopying.txt")
list(APPEND test_data "forRenaming.txt")
list(APPEND test_data "resources/file1.ext1")

qt_internal_add_test(tst_qfile
    SOURCES
        tst_qfile.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::TestPrivate
    TESTDATA ${test_data}
)

# Resources:
set(qfile_resource_files
    "resources/"
)

qt_internal_add_resource(tst_qfile "qfile"
    PREFIX
        "/tst_qfileinfo/"
    FILES
        ${qfile_resource_files}
)
set(rename-fallback_resource_files
    "rename-fallback.qrc"
)

qt_internal_add_resource(tst_qfile "rename-fallback"
    PREFIX
        "/"
    FILES
        ${rename-fallback_resource_files}
)
set(copy-fallback_resource_files
    "copy-fallback.qrc"
)

qt_internal_add_resource(tst_qfile "copy-fallback"
    PREFIX
        "/"
    FILES
        ${copy-fallback_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qfile CONDITION TARGET Qt::Network
    LIBRARIES
        Qt::Network
)

qt_internal_extend_target(tst_qfile CONDITION NOT TARGET Qt::Network
    DEFINES
        QT_NO_NETWORK
)

qt_internal_extend_target(tst_qfile CONDITION CONFIG___contains___builtin_testdata
    DEFINES
        BUILTIN_TESTDATA
)

qt_internal_extend_target(tst_qfile CONDITION WIN32
    LIBRARIES
        ole32
        uuid
)
add_subdirectory(stdinprocess)
if(QT_FEATURE_process)
    add_dependencies(tst_qfile stdinprocess_helper)
endif()
