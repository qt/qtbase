TEMPLATE = subdirs

!winrt {
    test.depends = app
    SUBDIRS += app
}

SUBDIRS += test
