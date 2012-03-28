QT = core-private
TEMPLATE = subdirs
CONFIG  += ordered
SUBDIRS = \
    plugin1 \
    plugin2 \
    test

TARGET = tst_qpluginloader

# no special install rule for subdir
INSTALLS =


CONFIG += parallel_test
