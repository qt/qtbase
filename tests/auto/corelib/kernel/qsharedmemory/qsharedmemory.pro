TEMPLATE = subdirs

qtConfig(sharedmemory) {
    !winrt: SUBDIRS = producerconsumer
    SUBDIRS += test.pro
}
