CONFIG += testcase

QT += widgets widgets-private
QT += core-private gui testlib

SOURCES		+= tst_qfilesystemmodel.cpp
TARGET		= tst_qfilesystemmodel

win32:CONFIG += insignificant_test # QTBUG-24291
