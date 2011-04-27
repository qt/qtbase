############################################################
# Project file for autotest for file qtoolbutton.h
############################################################

load(qttest_p4)

SOURCES += tst_qtoolbutton.cpp
contains(QT_CONFIG, qt3support): QT += qt3support

                                       

