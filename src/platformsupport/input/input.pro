TARGET = QtInputSupport
MODULE = input_support

QT = core-private gui-private devicediscovery_support-private
CONFIG += static internal_module

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../../corelib/global/qt_pch.h

qtConfig(evdev) {
    include($$PWD/evdevmouse/evdevmouse.pri)
    include($$PWD/evdevkeyboard/evdevkeyboard.pri)
    include($$PWD/evdevtouch/evdevtouch.pri)
    qtConfig(tabletevent) {
        include($$PWD/evdevtablet/evdevtablet.pri)
    }
}

qtConfig(tslib) {
    include($$PWD/tslib/tslib.pri)
}

qtConfig(libinput) {
    include($$PWD/libinput/libinput.pri)
}

qtConfig(evdev)|qtConfig(libinput) {
    include($$PWD/shared/shared.pri)
}

qtConfig(integrityhid) {
    include($$PWD/integrityhid/integrityhid.pri)
}

load(qt_module)
