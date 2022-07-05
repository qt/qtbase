# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from platformsupport.pro.

add_subdirectory(devicediscovery)
add_subdirectory(fbconvenience)
if(QT_FEATURE_evdev OR QT_FEATURE_integrityhid OR QT_FEATURE_libinput OR QT_FEATURE_tslib OR QT_FEATURE_xkbcommon)
    add_subdirectory(input)
endif()
if(QT_FEATURE_kms)
    add_subdirectory(kmsconvenience)
endif()
