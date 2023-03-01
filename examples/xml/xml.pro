TEMPLATE      = subdirs
SUBDIRS       = xmlstreamlint

qtHaveModule(widgets) {
    SUBDIRS +=  dombookmarks \
                streambookmarks

    qtHaveModule(network): SUBDIRS += \
                rsslisting
}
