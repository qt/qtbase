QT_FOR_CONFIG += widgets
requires(qtConfig(inputdialog))

TEMPLATE = subdirs
SUBDIRS = plugins app

app.depends = plugins
