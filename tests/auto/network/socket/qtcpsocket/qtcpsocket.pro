TEMPLATE = subdirs

SUBDIRS = test
!vxworks: SUBDIRS += stressTest

requires(contains(QT_CONFIG,private_tests))
