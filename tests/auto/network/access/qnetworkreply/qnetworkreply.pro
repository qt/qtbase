TEMPLATE = subdirs

!winrt:SUBDIRS += echo
test.depends += $$SUBDIRS
SUBDIRS += test
