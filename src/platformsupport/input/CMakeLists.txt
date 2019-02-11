# Generated from input.pro.

find_package(Libinput)
set_package_properties(Libinput PROPERTIES TYPE OPTIONAL)

find_package(XKB)
set_package_properties(XKB PROPERTIES TYPE OPTIONAL)

find_package(Tslib)
set_package_properties(Tslib PROPERTIES TYPE OPTIONAL)

#####################################################################
## InputSupport Module:
#####################################################################

add_qt_module(InputSupport
    STATIC
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::DeviceDiscoverySupportPrivate
    # CONFIG = "static" "internal_module"
    # MODULE = "input_support"
    # PRECOMPILED_HEADER = "../../corelib/global/qt_pch.h"
    # _LOADED = "qt_module"
)

## Scopes:
#####################################################################

extend_target(InputSupport CONDITION QT_FEATURE_evdev
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

extend_target(InputSupport CONDITION QT_FEATURE_evdev AND QT_FEATURE_tabletevent
    SOURCES
        evdevtablet/qevdevtablethandler.cpp evdevtablet/qevdevtablethandler_p.h
        evdevtablet/qevdevtabletmanager.cpp evdevtablet/qevdevtabletmanager_p.h
)

extend_target(InputSupport CONDITION QT_FEATURE_evdev AND QT_FEATURE_libudev AND QT_FEATURE_tabletevent
    LIBRARIES
        PkgConfig::Libudev
)

extend_target(InputSupport CONDITION QT_FEATURE_evdev AND QT_FEATURE_libudev
    LIBRARIES
        PkgConfig::Libudev
)

extend_target(InputSupport CONDITION QT_FEATURE_evdev AND QT_FEATURE_mtdev
    LIBRARIES
        mtdev
)

extend_target(InputSupport CONDITION QT_FEATURE_tslib
    SOURCES
        tslib/qtslib.cpp tslib/qtslib_p.h
    LIBRARIES
        PkgConfig::Tslib
)

extend_target(InputSupport CONDITION QT_FEATURE_libinput
    SOURCES
        libinput/qlibinputhandler.cpp libinput/qlibinputhandler_p.h
        libinput/qlibinputkeyboard.cpp libinput/qlibinputkeyboard_p.h
        libinput/qlibinputpointer.cpp libinput/qlibinputpointer_p.h
        libinput/qlibinputtouch.cpp libinput/qlibinputtouch_p.h
    INCLUDE_DIRECTORIES
        libinput/../shared
    LIBRARIES
        PkgConfig::Libudev
        Libinput::Libinput
)

extend_target(InputSupport CONDITION QT_FEATURE_libinput AND QT_FEATURE_xkbcommon_evdev
    LIBRARIES
        XKB::XKB
)

extend_target(InputSupport CONDITION QT_FEATURE_libinput AND NOT QT_FEATURE_xkbcommon_evdev
    DEFINES
        QT_NO_XKBCOMMON_EVDEV
)

extend_target(InputSupport CONDITION QT_FEATURE_evdev OR QT_FEATURE_libinput
    SOURCES
        shared/qtouchoutputmapping.cpp shared/qtouchoutputmapping_p.h
)

extend_target(InputSupport CONDITION QT_FEATURE_integrityhid
    SOURCES
        integrityhid/qintegrityhidmanager.cpp integrityhid/qintegrityhidmanager.h
    LIBRARIES
        integrityhid
)
