TEMPLATE = subdirs
SUBDIRS = \
    cbordump \
    convert \
    savegame

qtHaveModule(widgets) {
    qtHaveModule(network): SUBDIRS += \
                rsslisting
}
