TEMPLATE = subdirs
TARGET = widgets_snippets
SUBDIRS =

#! [qmake_use]
QT += widgets
#! [qmake_use]

contains(QT_BUILD_PARTS, tests) {
    SUBDIRS += \
        customviewstyle \
        filedialogurls \
        graphicssceneadditem \
        graphicsview \
        mdiarea \
        myscrollarea
}

