TARGET  = qtaccessiblewidgets

QT += core-private gui-private widgets-private

QTDIR_build:REQUIRES += "qtConfig(accessibility)"

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

PLUGIN_TYPE = accessible
PLUGIN_EXTENDS = widgets
PLUGIN_CLASS_NAME = AccessibleFactory
load(qt_plugin)
