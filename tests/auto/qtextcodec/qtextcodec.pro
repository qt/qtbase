TEMPLATE = subdirs
SUBDIRS = test

!wince*:!symbian:SUBDIRS += echo
