TEMPLATE = subdirs

!android:!winrt {
    test.depends = app
    SUBDIRS += app
}

SUBDIRS += test
