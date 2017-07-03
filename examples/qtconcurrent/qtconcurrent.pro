requires(qtHaveModule(concurrent))

TEMPLATE      = subdirs
SUBDIRS       = imagescaling \
                map \
                progressdialog \
                runfunction \
                wordcount


!qtHaveModule(gui) {
    SUBDIRS -= \
        map
}

!qtHaveModule(widgets) {
    SUBDIRS -= \
        imagescaling \
        progressdialog \
        runfunction \
        wordcount
}
