CONFIG += testcase
CONFIG += parallel_test
QT += widgets widgets-private
QT += gui-private core-private testlib

SOURCES		+= tst_qcolumnview.cpp
HEADERS         += ../../../../shared/fakedirmodel.h
TARGET		= tst_qcolumnview
