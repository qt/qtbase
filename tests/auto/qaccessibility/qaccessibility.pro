load(qttest_p4)
SOURCES  += tst_qaccessibility.cpp

unix:!mac:LIBS+=-lm

wince*: {
	accessneeded.files = $$QT_BUILD_TREE\\plugins\\accessible\\*.dll
	accessneeded.path = accessible
	DEPLOYMENT += accessneeded
}
