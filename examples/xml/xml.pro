TEMPLATE      = subdirs
SUBDIRS       = htmlinfo \
                xmlstreamlint

qtHaveModule(widgets) {
    SUBDIRS +=  dombookmarks \
                saxbookmarks \
                streambookmarks

    qtHaveModule(network): SUBDIRS += \
                rsslisting
}
