load(qttest_p4)
TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = qpong test

QT += core-private

requires(contains(QT_CONFIG,private_tests))
