TEMPLATE = subdirs

SUBDIRS = test
!vxworks: SUBDIRS += stressTest

requires(qtConfig(private_tests))
