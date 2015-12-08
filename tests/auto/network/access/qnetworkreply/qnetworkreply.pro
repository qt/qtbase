TEMPLATE = subdirs

!winrt:!wince: SUBDIRS += echo
test.depends += $$SUBDIRS
SUBDIRS += test
