contains(QT_CONFIG, dbus) {
    !mac:unix: {
        include($$PWD/ibus/ibus.pri)
    }
}
