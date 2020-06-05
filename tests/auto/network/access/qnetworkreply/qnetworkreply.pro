TEMPLATE = subdirs

SUBDIRS += echo
test.depends += $$SUBDIRS
SUBDIRS += test
