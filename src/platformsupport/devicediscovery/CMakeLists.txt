# Generated from devicediscovery.pro.

#####################################################################
## DeviceDiscoverySupport Module:
#####################################################################

qt_add_module(DeviceDiscoverySupport
    CONFIG_MODULE_NAME devicediscovery_support # special case
    STATIC
    INTERNAL_MODULE
    SOURCES
        qdevicediscovery_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

#### Keys ignored in scope 1:.:.:devicediscovery.pro:<TRUE>:
# MODULE = "devicediscovery_support"
# QT_FOR_CONFIG = "gui-private"

## Scopes:
#####################################################################

qt_extend_target(DeviceDiscoverySupport CONDITION QT_FEATURE_libudev
    SOURCES
        qdevicediscovery_udev.cpp qdevicediscovery_udev_p.h
    LIBRARIES
        PkgConfig::Libudev
)

qt_extend_target(DeviceDiscoverySupport CONDITION QT_FEATURE_evdev AND NOT QT_FEATURE_libudev
    SOURCES
        qdevicediscovery_static.cpp qdevicediscovery_static_p.h
)

qt_extend_target(DeviceDiscoverySupport CONDITION NOT QT_FEATURE_evdev AND NOT QT_FEATURE_libudev
    SOURCES
        qdevicediscovery_dummy.cpp qdevicediscovery_dummy_p.h
)
