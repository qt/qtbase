TEMPLATE = subdirs
SUBDIRS = \
    cbordump \
    convert \
    savegame

qtHaveModule(widgets) {
    SUBDIRS +=  streambookmarks
}
