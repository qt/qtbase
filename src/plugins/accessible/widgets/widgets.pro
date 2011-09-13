TARGET  = qtaccessiblewidgets
load(qt_plugin)
include (../qaccessiblebase.pri)

QT += core-private gui-private widgets-private
DESTDIR = $$QT.gui.plugins/accessible

QTDIR_build:REQUIRES += "contains(QT_CONFIG, accessibility)"

SOURCES  += main.cpp \
            simplewidgets.cpp \
            rangecontrols.cpp \
            complexwidgets.cpp \
            qaccessiblewidgets.cpp \
            qaccessiblemenu.cpp \
            itemviews.cpp

HEADERS  += qaccessiblewidgets.h \
            simplewidgets.h \
            rangecontrols.h \
            complexwidgets.h \
            qaccessiblemenu.h \
            itemviews.h


