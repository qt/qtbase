# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_find_package(Libinput MODULE)
qt_find_package(XKB MODULE)
qt_find_package(Tslib MODULE)
qt_find_package(Mtdev MODULE)

#####################################################################
## InputSupportPrivate Module:
#####################################################################

qt_internal_add_module(InputSupportPrivate
    CONFIG_MODULE_NAME input_support
    STATIC
    INTERNAL_MODULE
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::DeviceDiscoverySupportPrivate
        Qt::GuiPrivate
    PRECOMPILED_HEADER
        "../../corelib/global/qt_pch.h"
    NO_GENERATE_CPP_EXPORTS
)

## Scopes:
#####################################################################

qt_internal_extend_target(InputSupportPrivate CONDITION QT_FEATURE_evdev
    SOURCES
        evdevkeyboard/qevdevkeyboard_defaultmap_p.h
        evdevkeyboard/qevdevkeyboardhandler.cpp evdevkeyboard/qevdevkeyboardhandler_p.h
        evdevkeyboard/qevdevkeyboardmanager.cpp evdevkeyboard/qevdevkeyboardmanager_p.h
        evdevmouse/qevdevmousehandler.cpp evdevmouse/qevdevmousehandler_p.h
        evdevmouse/qevdevmousemanager.cpp evdevmouse/qevdevmousemanager_p.h
        evdevtouch/qevdevtouchhandler.cpp evdevtouch/qevdevtouchhandler_p.h
        evdevtouch/qevdevtouchmanager.cpp evdevtouch/qevdevtouchmanager_p.h
    INCLUDE_DIRECTORIES
        evdevtouch/../shared
)

qt_internal_extend_target(InputSupportPrivate CONDITION QT_FEATURE_evdev AND QT_FEATURE_tabletevent
    SOURCES
        evdevtablet/qevdevtablethandler.cpp evdevtablet/qevdevtablethandler_p.h
        evdevtablet/qevdevtabletmanager.cpp evdevtablet/qevdevtabletmanager_p.h
)

qt_internal_extend_target(InputSupportPrivate CONDITION QT_FEATURE_evdev AND QT_FEATURE_libudev AND QT_FEATURE_tabletevent
    LIBRARIES
        PkgConfig::Libudev
)

qt_internal_extend_target(InputSupportPrivate CONDITION QT_FEATURE_evdev AND QT_FEATURE_libudev
    LIBRARIES
        PkgConfig::Libudev
)

qt_internal_extend_target(InputSupportPrivate CONDITION QT_FEATURE_evdev AND QT_FEATURE_mtdev
    LIBRARIES
        PkgConfig::Mtdev
)

qt_internal_extend_target(InputSupportPrivate CONDITION QT_FEATURE_vxworksevdev
    SOURCES
        vxkeyboard/qvxkeyboard_defaultmap_p.h
        vxkeyboard/qvxkeyboardhandler.cpp vxkeyboard/qvxkeyboardhandler_p.h
        vxkeyboard/qvxkeyboardmanager.cpp vxkeyboard/qvxkeyboardmanager_p.h
        vxmouse/qvxmousehandler.cpp vxmouse/qvxmousehandler_p.h
        vxmouse/qvxmousemanager.cpp vxmouse/qvxmousemanager_p.h
        vxtouch/qvxtouchhandler.cpp vxtouch/qvxtouchhandler_p.h
        vxtouch/qvxtouchmanager.cpp vxtouch/qvxtouchmanager_p.h
    INCLUDE_DIRECTORIES
        shared
)

qt_internal_extend_target(InputSupportPrivate CONDITION QT_FEATURE_tslib
    SOURCES
        tslib/qtslib.cpp tslib/qtslib_p.h
    PUBLIC_LIBRARIES
        PkgConfig::Tslib
)

qt_internal_extend_target(InputSupportPrivate CONDITION QT_FEATURE_libinput
    SOURCES
        libinput/qlibinputhandler.cpp libinput/qlibinputhandler_p.h
        libinput/qlibinputkeyboard.cpp libinput/qlibinputkeyboard_p.h
        libinput/qlibinputpointer.cpp libinput/qlibinputpointer_p.h
        libinput/qlibinputtouch.cpp libinput/qlibinputtouch_p.h
    INCLUDE_DIRECTORIES
        libinput/../shared
    LIBRARIES
        Libinput::Libinput
        PkgConfig::Libudev
)

qt_internal_extend_target(InputSupportPrivate CONDITION QT_FEATURE_libinput AND QT_FEATURE_xkbcommon
    LIBRARIES
        XKB::XKB
)

qt_internal_extend_target(InputSupportPrivate CONDITION QT_FEATURE_evdev OR QT_FEATURE_libinput OR QT_FEATURE_vxworksevdev
    SOURCES
        shared/devicehandlerlist_p.h
        shared/qevdevutil.cpp shared/qevdevutil_p.h
        shared/qfdcontainer.cpp shared/qfdcontainer_p.h
        shared/qkeyboardmap_p.h
        shared/qkeycodeaction_p.h
        shared/qoutputmapping.cpp shared/qoutputmapping_p.h
        shared/qtouchfilter_p.h
)

qt_internal_extend_target(InputSupportPrivate CONDITION QT_FEATURE_integrityhid
    SOURCES
        integrityhid/qintegrityhidmanager.cpp integrityhid/qintegrityhidmanager.h
    LIBRARIES
        integrityhid
)
