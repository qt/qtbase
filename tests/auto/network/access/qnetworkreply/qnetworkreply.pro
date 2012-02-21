TEMPLATE = subdirs

!wince*:SUBDIRS += echo
test.depends += $$SUBDIRS
SUBDIRS += test
