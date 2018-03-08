TEMPLATE = subdirs
QT_FOR_CONFIG += network-private

!android:linux*:qtHaveModule(dbus) {
    SUBDIRS += generic
    SUBDIRS += connman networkmanager
}

android:!android-embedded: SUBDIRS += android

isEmpty(SUBDIRS):SUBDIRS = generic
