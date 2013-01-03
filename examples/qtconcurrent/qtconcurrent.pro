TEMPLATE      = subdirs
SUBDIRS       = imagescaling \
                map \
                runfunction \
                wordcount

!wince* {
    SUBDIRS += progressdialog
}

!qtHaveModule(widgets) {
    SUBDIRS -= \
        imagescaling \
        progressdialog \
        runfunction \
        wordcount
}
