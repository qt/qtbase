TARGET = QtDeviceDiscoverySupport
MODULE = devicediscovery_support

QT = core-private
QT_FOR_CONFIG += gui-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII

HEADERS += qdevicediscovery_p.h

qtConfig(libudev) {
    SOURCES += qdevicediscovery_udev.cpp
    HEADERS += qdevicediscovery_udev_p.h
    QMAKE_USE_PRIVATE += libudev
} else: qtConfig(evdev) {
    SOURCES += qdevicediscovery_static.cpp
    HEADERS += qdevicediscovery_static_p.h
} else {
    SOURCES += qdevicediscovery_dummy.cpp
    HEADERS += qdevicediscovery_dummy_p.h
}

load(qt_module)
