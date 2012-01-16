TEMPLATE = subdirs
SUBDIRS = copier paster
test.depends += $$SUBDIRS
SUBDIRS += test
