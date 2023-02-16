requires(qtHaveModule(concurrent))

TEMPLATE      = subdirs
SUBDIRS       = imagescaling \
                primecounter \
                wordcount

!qtHaveModule(widgets) {
    SUBDIRS -= \
        imagescaling \
        primecounter \
        wordcount
}
