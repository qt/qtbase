TEMPLATE = subdirs

include(selftests.pri)

selftest.file = selftest.pro
selftest.makefile = Makefile.selftest
selftest.target = selftest

SUBDIRS = $$SUBPROGRAMS selftest

INSTALLS =

QT = core
