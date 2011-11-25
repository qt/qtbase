CONFIG += testcase
TARGET = tst_qaccessibility
requires(contains(QT_CONFIG,accessibility))
QT += widgets testlib
SOURCES  += tst_qaccessibility.cpp

unix:!mac:LIBS+=-lm

wince*: {
	accessneeded.files = $$QT_BUILD_TREE\\plugins\\accessible\\*.dll
	accessneeded.path = accessible
	DEPLOYMENT += accessneeded
}

mac: CONFIG += insignificant_test # QTBUG-22812
