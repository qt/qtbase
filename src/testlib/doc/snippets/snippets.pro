TEMPLATE = subdirs
SUBDIRS =
contains(QT_BUILD_PARTS, tests) {
    SUBDIRS += \
        code
}

