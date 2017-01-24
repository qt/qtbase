TEMPLATE = subdirs

qtConfig(sharedmemory) {
    !winrt: SUBDIRS = sharedmemoryhelper
    SUBDIRS += test
}
