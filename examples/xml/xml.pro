TEMPLATE      = subdirs
SUBDIRS       = htmlinfo \
                xmlstreamlint

qtHaveModule(widgets) {
    SUBDIRS +=  dombookmarks \
                rsslisting \
                saxbookmarks \
                streambookmarks
}
