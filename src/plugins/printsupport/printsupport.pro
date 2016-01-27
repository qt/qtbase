TEMPLATE = subdirs

osx:   SUBDIRS += cocoa
win32: SUBDIRS += windows
unix:!mac:contains(QT_CONFIG, cups) {
    load(qfeatures)
    !contains(QT_DISABLED_FEATURES, cups): SUBDIRS += cups
}
