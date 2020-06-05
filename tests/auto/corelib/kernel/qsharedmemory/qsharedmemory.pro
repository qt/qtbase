TEMPLATE = subdirs

qtConfig(sharedmemory) {
    SUBDIRS = producerconsumer \
              test.pro
}
