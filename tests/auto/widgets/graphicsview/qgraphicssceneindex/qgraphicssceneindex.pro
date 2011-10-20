load(qttest_p4)
requires(contains(QT_CONFIG,private_tests))
QT += widgets widgets-private
QT += core-private gui-private
SOURCES  += tst_qgraphicssceneindex.cpp
CONFIG += parallel_test
