TEMPLATE      = subdirs
SUBDIRS       = htmlinfo \
                xmlstreamlint

qtHaveModule(widgets) {
    SUBDIRS +=  dombookmarks \
                streambookmarks

    qtHaveModule(network): SUBDIRS += \
                rsslisting
}
