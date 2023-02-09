# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(devicediscovery)
add_subdirectory(fbconvenience)
if(QT_FEATURE_evdev OR QT_FEATURE_integrityhid OR QT_FEATURE_libinput OR QT_FEATURE_tslib OR QT_FEATURE_xkbcommon)
    add_subdirectory(input)
endif()
if(QT_FEATURE_kms)
    add_subdirectory(kmsconvenience)
endif()
