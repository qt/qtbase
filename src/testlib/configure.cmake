# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause



#### Inputs



#### Libraries



#### Tests



#### Features

qt_feature("testlib_selfcover" PUBLIC
    LABEL "Coverage testing of testlib itself"
    PURPOSE "Gauges how thoroughly testlib's selftest exercises testlib's code"
    AUTODETECT OFF
)
qt_feature("itemmodeltester" PUBLIC
    LABEL "Tester for item models"
    PURPOSE "Provides a utility to test item models."
    CONDITION QT_FEATURE_itemmodel
)
qt_feature("valgrind" PUBLIC
    LABEL "Valgrind"
    PURPOSE "Profiling support with callgrind."
    CONDITION ( LINUX OR APPLE ) AND QT_FEATURE_process AND QT_FEATURE_regularexpression
)
qt_feature("batch_test_support" PUBLIC
    LABEL "Batch tests"
    PURPOSE "Allows merging of all tests into a single executable on demand"
    AUTODETECT QT_BUILD_TESTS_BATCHED
    ENABLE INPUT_batch_tests STREQUAL 'yes'
)
qt_configure_add_summary_section(NAME "Qt Testlib")
qt_configure_add_summary_entry(ARGS "itemmodeltester")
qt_configure_add_summary_entry(ARGS "batch_test_support")
qt_configure_end_summary_section() # end of "Qt Testlib" section
