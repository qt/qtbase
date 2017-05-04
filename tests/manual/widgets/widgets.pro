TEMPLATE = subdirs
SUBDIRS = itemviews qgraphicsview kernel widgets
greaterThan(QT_MAJOR_VERSION, 4): SUBDIRS += styles
