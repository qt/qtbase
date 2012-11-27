TEMPLATE      = subdirs
SUBDIRS       = htmlinfo \
                xmlstreamlint

!contains(QT_CONFIG, no-widgets) {
    SUBDIRS +=  dombookmarks \
                rsslisting \
                saxbookmarks \
                streambookmarks
}
