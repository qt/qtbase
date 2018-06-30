TEMPLATE = subdirs

!android:!winrt {
    test.depends = crashonexit
    SUBDIRS += crashonexit
}

SUBDIRS += test
