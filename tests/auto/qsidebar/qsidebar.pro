CONFIG += qttest_p4

QT += core-private
SOURCES		+= tst_qsidebar.cpp 
TARGET		= tst_qsidebar

symbian:HEADERS += ../../../include/qtgui/private/qfileinfogatherer_p.h
