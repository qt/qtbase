TEMPLATE = subdirs

SUBDIRS = desktopsettingsaware modal

test.depends += $$SUBDIRS
SUBDIRS += test
