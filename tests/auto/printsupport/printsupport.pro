TEMPLATE=subdirs
QT_FOR_CONFIG += printsupport
requires(qtConfig(printer))
SUBDIRS=\
    dialogs \
    kernel \
