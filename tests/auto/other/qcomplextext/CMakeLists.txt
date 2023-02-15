# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcomplextext Test:
#####################################################################

# Collect test data
list(APPEND test_data "data")

qt_internal_add_test(tst_qcomplextext
    SOURCES
        tst_qcomplextext.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

if(ANDROID)
    # Resources:
    set(android_testdata_resource_files
        "data/BidiCharacterTest.txt"
        "data/BidiTest.txt"
    )

    qt_internal_add_resource(tst_qcomplextext "android_testdata"
        PREFIX
            "/android_testdata"
        FILES
            ${android_testdata_resource_files}
    )
endif()

qt_internal_extend_target(tst_qcomplextext CONDITION builtin_testdata
    DEFINES
        BUILTIN_TESTDATA
)
