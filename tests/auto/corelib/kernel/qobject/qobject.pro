TEMPLATE = subdirs

!winrt {
    test.depends = signalbug
    SUBDIRS += signalbug
}

SUBDIRS += test
