TEMPLATE = subdirs
SUBDIRS = itemviews qgraphicsview kernel
greaterThan(QT_MAJOR_VERSION, 4): SUBDIRS += styles
