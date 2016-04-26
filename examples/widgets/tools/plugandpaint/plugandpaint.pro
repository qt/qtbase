TEMPLATE = subdirs
SUBDIRS = plugins app

app.depends = plugins
