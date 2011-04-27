TARGET  = qtaccessiblewidgets
include(../../qpluginbase.pri)
include (../qaccessiblebase.pri)

DESTDIR = $$QT.gui.plugins/accessible

QTDIR_build:REQUIRES += "contains(QT_CONFIG, accessibility)"

SOURCES  += main.cpp \
	    simplewidgets.cpp \
	    rangecontrols.cpp \
	    complexwidgets.cpp \
	    qaccessiblewidgets.cpp \
	    qaccessiblemenu.cpp

HEADERS  += qaccessiblewidgets.h \
	    simplewidgets.h \
	    rangecontrols.h \
	    complexwidgets.h \
	    qaccessiblemenu.h
