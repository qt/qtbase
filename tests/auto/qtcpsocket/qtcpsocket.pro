TEMPLATE = subdirs


!wince*: SUBDIRS = test stressTest
wince*|symbian|vxworks* : SUBDIRS = test


requires(contains(QT_CONFIG,private_tests))

CONFIG+=insignificant_test  # unstable, QTBUG-21043
