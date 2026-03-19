# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(QT_FEATURE_developer_build)
    if (QT_FEATURE_datestring)
        add_subdirectory(qtparseqttemporalformat)
    endif()
    if(QT_FEATURE_datetimeparser)
        add_subdirectory(qtparsetemporal)
        add_subdirectory(qtparsetimezone)
    endif()
endif()
