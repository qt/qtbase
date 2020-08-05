TEMPLATE = subdirs
SUBDIRS =

contains(QT_BUILD_PARTS, tests) {
    SUBDIRS += \
        brush \
        clipboard \
        code \
        draganddrop \
        qfontdatabase \
        textdocument-end
}
