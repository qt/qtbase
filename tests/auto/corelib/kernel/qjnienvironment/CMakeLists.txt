# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qjnienvironment Test:
#####################################################################

qt_internal_add_test(tst_qjnienvironment
    SOURCES
        tst_qjnienvironment.cpp
)

if(ANDROID)
    set_property(TARGET tst_qjnienvironment APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
        ${CMAKE_CURRENT_SOURCE_DIR}/testdata
    )
endif()
