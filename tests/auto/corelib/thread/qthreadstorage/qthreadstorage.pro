TEMPLATE = subdirs
SUBDIRS = \
    test

!winrt: SUBDIRS += crashonexit

CONFIG += ordered
