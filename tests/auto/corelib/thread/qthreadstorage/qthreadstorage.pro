TEMPLATE = subdirs

!android {
    test.depends = crashonexit
    SUBDIRS += crashonexit
}

SUBDIRS += test
