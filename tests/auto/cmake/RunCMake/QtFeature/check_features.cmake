# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

foreach(feature IN LISTS expected_on)
    if(NOT QT_FEATURE_${feature})
        message(FATAL_ERROR "QT_FEATURE_${feature} should be ON")
    endif()
endforeach()
foreach(feature IN LISTS expected_off)
    if(QT_FEATURE_${feature})
        message(FATAL_ERROR "QT_FEATURE_${feature} should be OFF")
    endif()
endforeach()
