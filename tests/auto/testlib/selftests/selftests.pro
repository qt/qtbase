TEMPLATE = subdirs

!winrt: include(selftests.pri)

SUBDIRS = $$SUBPROGRAMS test

INSTALLS =

QT = core


CONFIG += parallel_test
