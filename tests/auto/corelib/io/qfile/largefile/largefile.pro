load(qttest_p4)

QT = core
SOURCES += tst_largefile.cpp

wince*: SOURCES += $$QT_SOURCE_TREE/src/corelib/kernel/qfunctions_wince.cpp

CONFIG += parallel_test
CONFIG += insignificant_test # QTBUG-21175
