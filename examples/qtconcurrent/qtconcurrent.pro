requires(qtHaveModule(concurrent))

TEMPLATE      = subdirs
SUBDIRS       = imagescaling \
                progressdialog \
                runfunction \
                wordcount

!qtHaveModule(widgets) {
    SUBDIRS -= \
        imagescaling \
        progressdialog \
        runfunction \
        wordcount
}
