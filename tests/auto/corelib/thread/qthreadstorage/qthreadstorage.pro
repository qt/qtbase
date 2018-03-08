TEMPLATE = subdirs

!winrt {
    test.depends = crashonexit
    SUBDIRS += crashonexit
}

SUBDIRS += test
