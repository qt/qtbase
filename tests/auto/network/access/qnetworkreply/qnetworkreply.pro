TEMPLATE = subdirs
SUBDIRS = test

requires(contains(QT_CONFIG,private_tests))

!wince*:SUBDIRS += echo
