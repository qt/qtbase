requires(qtHaveModule(concurrent))

TEMPLATE      = subdirs
SUBDIRS       = imagescaling \
                progressdialog \
                wordcount

!qtHaveModule(widgets) {
    SUBDIRS -= \
        imagescaling \
        progressdialog \
        wordcount
}
