QT = core-private
TEMPLATE = subdirs

test.depends = plugin1 plugin2
SUBDIRS = \
    plugin1 \
    plugin2 \
    test

TARGET = tst_qpluginloader

# no special install rule for subdir
INSTALLS =


