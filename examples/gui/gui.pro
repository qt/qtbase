TEMPLATE     = subdirs
CONFIG += no_docs_target

SUBDIRS += analogclock
SUBDIRS += rasterwindow
contains(QT_CONFIG, opengl(es1|es2)?) {
    SUBDIRS += openglwindow
}
