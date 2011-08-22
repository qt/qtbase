TEMPLATE = subdirs


!wince*: SUBDIRS = test stressTest
wince*|symbian|vxworks* : SUBDIRS = test


requires(contains(QT_CONFIG,private_tests))
