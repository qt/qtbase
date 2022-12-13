# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qjniobject Test:
#####################################################################

qt_internal_add_test(tst_qjniobject
    SOURCES
        tst_qjniobject.cpp
)

if(ANDROID)
    set_property(TARGET tst_qjniobject APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
        ${CMAKE_CURRENT_SOURCE_DIR}/testdata
    )
endif()
