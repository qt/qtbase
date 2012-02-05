TEMPLATE = subdirs

SUBDIRS = desktopsettingsaware modal

win32:!wince*:SUBDIRS += wincmdline
test.depends += $$SUBDIRS
SUBDIRS += test
