TEMPLATE = subdirs


!wince*: SUBDIRS = test stressTest
wince*|vxworks* : SUBDIRS = test

requires(contains(QT_CONFIG,private_tests))
