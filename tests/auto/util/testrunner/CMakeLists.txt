# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


# Run the qt-testrunner test only inside the CI.
if(DEFINED ENV{COIN_UNIQUE_JOB_ID} AND NOT IOS)
    qt_internal_create_test_script(
        NAME tst_qt_testrunner
        COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tst_qt_testrunner.py" ARGS -v
        WORKING_DIRECTORY "${test_working_dir}"
        OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/tst_qt_testrunner_Wrapper$<CONFIG>.cmake"
        ENVIRONMENT "TESTRUNNER" ""
    )
endif()
