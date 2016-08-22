TEMPLATE = subdirs

!android:linux*:qtHaveModule(dbus) {
    SUBDIRS += generic
    SUBDIRS += connman networkmanager
}

#win32:SUBDIRS += nla
win32:SUBDIRS += generic
win32:!winrt: SUBDIRS += nativewifi
darwin:qtConfig(corewlan): SUBDIRS += corewlan
mac:SUBDIRS += generic
android:SUBDIRS += android

isEmpty(SUBDIRS):SUBDIRS = generic
