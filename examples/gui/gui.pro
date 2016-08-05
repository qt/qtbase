requires(qtHaveModule(gui))

TEMPLATE     = subdirs
CONFIG += no_docs_target

SUBDIRS += analogclock
SUBDIRS += rasterwindow
qtConfig(opengl(es2)?) {
    SUBDIRS += openglwindow
}
