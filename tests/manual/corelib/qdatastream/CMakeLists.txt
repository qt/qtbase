# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_manual_qdatastream Test:
#####################################################################

qt_internal_add_manual_test(tst_manual_qdatastream
    SOURCES
        tst_manualqdatastream.cpp
    LIBRARIES
        Qt::Core
    ENABLE_AUTOGEN_TOOLS
        uic
)
