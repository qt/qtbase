requires(qtHaveModule(gui))

TEMPLATE     = subdirs
QT_FOR_CONFIG += gui
CONFIG += no_docs_target

SUBDIRS += analogclock
SUBDIRS += rasterwindow
qtConfig(opengl): \
    SUBDIRS += openglwindow
