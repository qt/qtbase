TEMPLATE = subdirs
qtHaveModule(concurrent) {
    SUBDIRS = \
        qmimedatabase-xml-builtin \
        qmimedatabase-xml-fdoxml

    unix:!darwin:!qnx: {
        SUBDIRS += \
            qmimedatabase-cache-builtin \
            qmimedatabase-cache-fdoxml
    }
}
