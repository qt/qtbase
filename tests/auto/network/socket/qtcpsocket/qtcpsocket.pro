TEMPLATE = subdirs

SUBDIRS = test
!wince:!vxworks: SUBDIRS += stressTest

requires(contains(QT_CONFIG,private_tests))
