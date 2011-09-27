TEMPLATE = subdirs
SUBDIRS = test # lackey should be moved to the QtScript module
!wince*: SUBDIRS += example
