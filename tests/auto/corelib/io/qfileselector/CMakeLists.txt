# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qfileselectors Test:
#####################################################################

# Resources:
set(qfileselector_resource_files
    "extras/+custom1/test"
    "extras/+custom1/test3"
    "extras/+custom2/test"
    "extras/+custom3/+custom2/test"
    "extras/+custom3/+custom4/test"
    "extras/+custom3/+custom5/test"
    "extras/+custom3/test"
    "extras/+custom5/+custom3/test"
    "extras/test"
    "extras/test2"
    "platforms/+android/test"
    "platforms/+android/test2"
    "platforms/+darwin/test"
    "platforms/+haiku/test"
    "platforms/+haiku/test2"
    "platforms/+ios/test"
    "platforms/+ios/test2"
    "platforms/+linux/test"
    "platforms/+linux/test2"
    "platforms/+macos/test"
    "platforms/+macos/test2"
    "platforms/+qnx/test"
    "platforms/+qnx/test2"
    "platforms/+unix/+android/test"
    "platforms/+unix/+darwin/+ios/test"
    "platforms/+unix/+darwin/+macos/test"
    "platforms/+unix/+darwin/test"
    "platforms/+unix/+haiku/test"
    "platforms/+unix/+linux/test"
    "platforms/+unix/+qnx/test"
    "platforms/+unix/test"
    "platforms/+unix/test3"
    "platforms/+wince/test"
    "platforms/+wince/test2"
    "platforms/+windows/+wince/test"
    "platforms/+windows/+winnt/test"
    "platforms/+windows/+winrt/test"
    "platforms/+windows/test"
    "platforms/+windows/test3"
    "platforms/+winnt/test2"
    "platforms/+winrt/test"
    "platforms/+winrt/test2"
    "platforms/test"
    "platforms/test2"
    "platforms/test3"
    "platforms/test4"
    "platforms/test5"
)

qt_internal_add_test(tst_qfileselectors
    SOURCES
        tst_qfileselector.cpp
    LIBRARIES
        Qt::CorePrivate
    TESTDATA ${qfileselector_resource_files}
    BUILTIN_TESTDATA
)

