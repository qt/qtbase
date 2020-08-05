TEMPLATE = subdirs
SUBDIRS =

contains(QT_BUILD_PARTS, tests) {
    SUBDIRS += \
        brush \
        clipboard \
        code \
        draganddrop \
        qfontdatabase \
        textdocument-blocks \
        textdocument-charformats \
        textdocument-css \
        textdocument-cursors \
        textdocument-end \
        textdocument-find \
        textdocument-frames \
        textdocument-imagedrop \
        textdocument-imageformat \
        textdocument-images \
        textdocument-listitems
}
