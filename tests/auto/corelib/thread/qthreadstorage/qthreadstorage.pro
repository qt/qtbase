TEMPLATE = subdirs
SUBDIRS = \
    test

!winrt: SUBDIRS += crashonexit

CONFIG += ordered parallel_test
