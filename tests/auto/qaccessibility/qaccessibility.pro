load(qttest_p4)
SOURCES  += tst_qaccessibility.cpp

unix:!mac:LIBS+=-lm
contains(QT_CONFIG, qt3support): QT += qt3support

wince*: {
	accessneeded.files = $$QT_BUILD_TREE\\plugins\\accessible\\*.dll
	accessneeded.path = accessible
	DEPLOYMENT += accessneeded
}