CONFIG += testcase
TARGET = tst_qaccessibility
requires(contains(QT_CONFIG,accessibility))
QT += testlib core-private gui-private widgets-private
SOURCES += tst_qaccessibility.cpp
HEADERS += accessiblewidgets.h

unix:!mac:!haiku:LIBS+=-lm

wince {
	accessneeded.files = $$QT_BUILD_TREE\\plugins\\accessible\\*.dll
	accessneeded.path = accessible
	DEPLOYMENT += accessneeded
}

win32 {
    !*g++:!winrt {
        include(../../../../src/3rdparty/iaccessible2/iaccessible2.pri)
        DEFINES += QT_SUPPORTS_IACCESSIBLE2
    }
    LIBS += -luuid
    !winphone: LIBS += -loleacc -loleaut32 -lole32
}
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
