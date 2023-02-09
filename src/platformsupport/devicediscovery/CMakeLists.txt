# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## DeviceDiscoverySupportPrivate Module:
#####################################################################

qt_internal_add_module(DeviceDiscoverySupportPrivate
    CONFIG_MODULE_NAME devicediscovery_support
    STATIC
    INTERNAL_MODULE
    SOURCES
        qdevicediscovery_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(DeviceDiscoverySupportPrivate CONDITION QT_FEATURE_libudev
    SOURCES
        qdevicediscovery_udev.cpp qdevicediscovery_udev_p.h
    LIBRARIES
        PkgConfig::Libudev
)

qt_internal_extend_target(DeviceDiscoverySupportPrivate CONDITION QT_FEATURE_evdev AND NOT QT_FEATURE_libudev
    SOURCES
        qdevicediscovery_static.cpp qdevicediscovery_static_p.h
)

qt_internal_extend_target(DeviceDiscoverySupportPrivate CONDITION NOT QT_FEATURE_evdev AND NOT QT_FEATURE_libudev
    SOURCES
        qdevicediscovery_dummy.cpp qdevicediscovery_dummy_p.h
)
