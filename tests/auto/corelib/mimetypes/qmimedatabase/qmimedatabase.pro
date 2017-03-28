TEMPLATE = subdirs
qtHaveModule(concurrent) {
    SUBDIRS = qmimedatabase-xml
    unix:!darwin:!qnx: SUBDIRS += qmimedatabase-cache
}
