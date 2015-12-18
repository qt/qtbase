TEMPLATE = subdirs
!winrt: SUBDIRS = copier paster
test.depends += $$SUBDIRS
SUBDIRS += test
