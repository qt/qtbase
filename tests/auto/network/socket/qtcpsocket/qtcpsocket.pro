TEMPLATE = subdirs


!wince*: SUBDIRS = test stressTest
wince*|vxworks* : SUBDIRS = test

linux-*:system(". /etc/lsb-release && [ $DISTRIB_CODENAME = oneiric ]"):DEFINES+=UBUNTU_ONEIRIC

requires(contains(QT_CONFIG,private_tests))
