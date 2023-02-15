# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qstandardpaths Test:
#####################################################################

# Collect test data
list(APPEND test_data "tst_qstandardpaths.cpp")

qt_internal_add_test(tst_qstandardpaths
    SOURCES
        tst_qstandardpaths.cpp
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qstandardpaths CONDITION boot2qt
    DEFINES
        SKIP_FINDEXECUTABLE
)
