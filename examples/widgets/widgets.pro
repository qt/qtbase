requires(qtHaveModule(widgets))

TEMPLATE      = subdirs
CONFIG += no_docs_target

SUBDIRS       = \
                animation \
                desktop \
                dialogs \
                draganddrop \
                effects \
                gestures \
                graphicsview \
                itemviews \
                layouts \
                mainwindows \
                painting \
                richtext \
                scroller \
                statemachine \
                tools \
                tutorials \
                widgets

qtConfig(opengl): \
    SUBDIRS += windowcontainer

contains(DEFINES, QT_NO_CURSOR): SUBDIRS -= mainwindows
contains(DEFINES, QT_NO_DRAGANDDROP): SUBDIRS -= draganddrop
mac:SUBDIRS += mac
