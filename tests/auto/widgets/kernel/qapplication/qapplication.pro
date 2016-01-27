TEMPLATE = subdirs

!winrt: SUBDIRS = desktopsettingsaware modal

test.depends += $$SUBDIRS
SUBDIRS += test
