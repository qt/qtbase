# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_uic Test:
#####################################################################

qt_internal_add_test(tst_uic
    SOURCES
        tst_uic.cpp
)

# Add dependencies that are implicitly used inside the test
add_dependencies(tst_uic
    Qt::uic
)
