requires(qtHaveModule(concurrent))

TEMPLATE      = subdirs
SUBDIRS       = imagescaling \
                map \
                runfunction \
                wordcount

!wince* {
    SUBDIRS += progressdialog
}

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
