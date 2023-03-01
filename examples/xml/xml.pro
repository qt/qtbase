TEMPLATE      = subdirs

qtHaveModule(widgets) {
    SUBDIRS +=  dombookmarks \
                streambookmarks

    qtHaveModule(network): SUBDIRS += \
                rsslisting
}
