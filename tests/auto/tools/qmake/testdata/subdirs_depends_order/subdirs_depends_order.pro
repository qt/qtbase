TEMPLATE = subdirs

# mainapp is listed before the library it depends on. qmake must reorder the
# subdirs according to .depends so that deplib is processed and built first
# (QTBUG-45706). CONFIG += ordered is deliberately not used.
SUBDIRS = \
    mainapp \
    deplib

mainapp.depends = deplib
