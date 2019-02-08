# Generated from devicediscovery.pro.

#####################################################################
## DeviceDiscoverySupport Module:
#####################################################################

add_qt_module(DeviceDiscoverySupport
    STATIC
    SOURCES
        qdevicediscovery_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

extend_target(DeviceDiscoverySupport CONDITION QT_FEATURE_libudev
    SOURCES
        qdevicediscovery_udev.cpp qdevicediscovery_udev_p.h
    LIBRARIES
        libudev
)

extend_target(DeviceDiscoverySupport CONDITION QT_FEATURE_evdev AND NOT QT_FEATURE_libudev
    SOURCES
        qdevicediscovery_static.cpp qdevicediscovery_static_p.h
)

extend_target(DeviceDiscoverySupport CONDITION NOT QT_FEATURE_evdev AND NOT QT_FEATURE_libudev
    SOURCES
        qdevicediscovery_dummy.cpp qdevicediscovery_dummy_p.h
)
